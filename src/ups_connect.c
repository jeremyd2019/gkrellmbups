/*      __       __
 *   __/ /_______\ \__     ___ ___ __ _                       _ __ ___ ___
 *__/ / /  .---.  \ \ \___/                                               \___
 *_/ | '  /  / /\  ` | \_/          (C) Copyright 2003, Chris Page         \__
 * \ | |  | / / |  | | / \  Released under the GNU General Public License  /
 *  >| .  \/ /  /  . |<   >--- --- -- -                       - -- --- ---<
 * / \_ \  `/__'  / _/ \ /  This program is free software released under   \
 * \ \__ \_______/ __/ / \   the GNU GPL. Please see the COPYING file in   /
 *  \  \_         _/  /   \   the distribution archive for more details   /
 * //\ \__  ___  __/ /\\ //\                                             /
 *- --\  /_/   \_\  /-- - --\                                           /-----
 *-----\_/       \_/---------\   ___________________________________   /------
 *                            \_/                                   \_/
 */
/** 
 *
 *  \file ups_connect.c
 *  UPS service connection and parsing functions.
 *  This file contains the code which connects to the upsd service and parses
 *  the results from the connection into UPSData structures. Belkin Sentry
 *  Dulldog or optional NUT conections are supported.
 *
 *  I've no idea if this information is available anywhere but the contents of
 *  this comment have been derived by reverse enginerring of the protocol so
 *  expect errors. A thorough search with google and a manual search of the 
 *  Belkin site turned up no information on the Belkin Sentry Bulldog upsd 
 *  protocol hence some of this is guesswork!  
 *
 *  Data obtained from the upsd service takes the form of tab seperated data 
 *  fields of the generic form 
 *
 *  DeltaUPS:<code>,00,<flag> <TSVs>
 *
 *  When a program initially connects to the ups service (localhost.2710 from
 *  the local machine) a sigificant amount of general status information is 
 *  sent before the actual UPS status data. Much of this appears to be echoed
 *  versions of the PRO_* files in the bulldog directory. The values we are
 *  interested in start DeltaUPS:VAL00,00. Here are the fields I have been able
 *  to positively identify:
 * <PRE>
 * DeltaUPS:VAL00,00,xxxx P\t  xxxx is of unknown purpose. P=1 if UPS present
 *                             otherwise P = 0 (I think!)
 * field 1 to 4 (usually 1\t536915997\t603979777\t8\t)     unknown, serial number?
 * field 5                                                 Battery voltage
 * field 6                                                 unknown
 * field 7                                                 Battery level
 * field 8                                                 Input frequency
 * field 9                                                 Input voltage
 * fields 10 to 16 (usually -1\t)                          unknown
 * field 17                                                Output frequency
 * field 18                                                Output voltage
 * field 19 (usually -1\t)                                 unknown
 * field 20                                                Load level
 * fields 21 to 33 (usually -1\t)                          unknown
 * field 34                                                Temperature
 * Purpose of all later fields is unknown.
 * </PRE>
 * Log entries appear to be of the form
 *
 * DeltaUPS:LOG00,00,xxxx 0 <date> <time> '<'<message>'>'
 *
 * Log entries are followed by a CR+LF sequence (?)
 *
 * \sa 
 * http://www.exploits.org/nut/ - NUT homepage <BR>
 * http://www.belkin.com/       - UPS details <BR>
 * Unix Network Proramming Vol1, 2nd Ed (ISBN 0-13-490012-X) - network programming
 * reference
 * 
 */
/* $Id: ups_connect.c,v 1.2 2003/02/06 21:07:53 chris Exp $
 */

#include<glib.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/select.h>
#include<sys/socket.h>
#include<netdb.h>
#include"gkrellmbups.h"
#include"ups_connect.h"
#include"../config.h"

struct UPSData  ups_status; /*!< Global UPS data structure, must be synchronised across threads! */
GMutex         *ups_status_lock = NULL; /*!< Synchronisation mutex for ups_status */

static gboolean haltThread = FALSE; /*!< Used to shut down the client thread from gkrellm, set to TRUE to halt then g_thread_join */

/* Status strings used mainly in ups_connect */
static const gchar noUPS[]      = "UPS not connected";
static const gchar gotUPS[]     = "UPS monitoring active";
static const gchar noMem[]      = "Out of memory";
static const gchar noSock[]     = "Socket error";
static const gchar badHost[]    = "Unable to find host";
static const gchar badConn[]    = "Connection refused";
static const gchar connLost[]   = "Connection to UPS lost";
static const gchar disconHost[] = "Disconnecting from server";


#ifdef ENABLE_NUT
/*! Status code to human-readable message loop-up table 
 */
static struct
{
    gchar *name;
    gchar *log;
} status_texts[] = 
{
    {   "OFF", "UPS is offline"},
    {    "OL", "UPS online, utility up"},
    {    "OB", "UPS on battery backup"},
    {    "LB", "Low battery warning!"},
    {    "RB", "Replace the UPS battery"},
    {  "OVER", "UPS is overloaded"},
    {  "TRIM", "Trimming voltage"},
    { "BOOST", "Boosting voltage"},
    {   "CAL", "Calibrating"},
    {    NULL, NULL }
};

/*! Variables that are relevant to the plugin, whether the UPS
 *  supports it and the float the variable should go in.
 */
/* FIXME: Could be local to ups_client_nut ? */
static struct 
{
    gchar    *name;
    gboolean  avail;
    gfloat   *var;
} ups_vars[] =
{
    {  "STATUS", FALSE, NULL }, /* not handled by read loop */
    { "UTILITY", FALSE, &ups_status.in_Voltage },
    { "BATTPCT", FALSE, &ups_status.bat_Level },
    {  "ACFREQ", FALSE, &ups_status.in_Freq },
    { "UPSTEMP", FALSE, &ups_status.ups_Temp },
    { "LOADPCT", FALSE, &ups_status.ups_Load },
    {"BATTVOLT", FALSE, &ups_status.bat_Voltage },
    { "OUTVOLT", FALSE, &ups_status.out_Voltage },
    {      NULL, FALSE, NULL}    
};

#define VAR_STATUS 0

#endif

/* Globals used to simplify thread startup code.
 */
static gchar *pro_net_loc = NULL;
static gchar *ups_host    = NULL;
static gint   ups_port;
static gint   ups_mode;


/*****************************************************************************\
* Status structure update and utility functions.                              *
\*****************************************************************************/ 

/** Process the bulldog PRO_NET file to extract the port to connect to.
 *  When the user has selected a local system all we need to know is the
 *  port to connect to - this is the second line in the PRO_NET.DAT file
 *  written by the belkin upsd server.
 */
gint process_pronet(gchar *filename)
{
    FILE *pronet;
    gchar buffer[MAX_PRONET], *remain;
    gint value;

    if(strlen(filename)) {
        if((pronet = fopen(filename, "r")) != NULL) {
            fgets(buffer, MAX_PRONET, pronet); /* read the first line (master/slave) */
            fgets(buffer, MAX_PRONET, pronet); /* this is the line we want - the port */
            if((*buffer != '\n') && (*buffer != '\0')) {
                value = strtol(buffer, &remain, 0);
                if(remain != buffer) {
                    return value;
                }
            }
            fclose(pronet);
        }
    }

    return 0;
}


/** Clear the specified UPSData structure. 
 *  Use this to zero all the fields of a UPSData structure. Mainly intended to 
 *  simplify the initialisation of static structures.
 */
static void reset_status(struct UPSData *target)
{
    target -> bat_Voltage = 0.0;
    target -> bat_Level   = 0.0;
    target -> in_Freq     = 0.0;
    target -> in_Voltage  = 0.0;
    target -> out_Freq    = 0.0;
    target -> out_Voltage = 0.0;
    target -> ups_Load    = 0.0;
    target -> ups_Temp    = 0.0;
    target -> ups_LastLog[0] = '\0';
    target -> ups_Present = FALSE;
}


/** Set the ups_LastLog field of a UPSData structure.
 *  Setting the ups_LastLog is slightly more work than using strcpy as I
 *  want to ensure that the buffer can not overflow (fairly vital as the
 *  socket fd is after the buffer in memory!). This uses strncpy, checks
 *  that the buffer is not overflowed and ensures that the string is 
 *  null terminated.
 *
 *  \par Arguments:
 *  \arg \c target - UPSData structure containing the log field to set.
 *  \arg \c log - String to set the ups_LastLog field to.
 */
static void set_last_log(struct UPSData *target, const gchar *log)
{
    gint size;

    size = MIN(strlen(log), MAX_LOGSIZE - 1);
    strncpy(target -> ups_LastLog, log, size);
    target -> ups_LastLog[size] = 0;
}


/** Traverse string until a number of tabs have been encountered.
 *  The data generated by the upsd service is basically tab separated and
 *  most of the fields in the data appear to serve no purpose. This
 *  function allows a number of tabs to be skipped in one go, reducing
 *  the amount of waffle needed in parseVAL().
 *
 *  \par Arguments:
 *  \arg \c str - String to be processed.
 *  \arg \c count - Number of tags to skip.
 *  \return A pointer to the character after the last tab skipped, or the
 *  end of the string (if the string ended before enough tabs had been
 *  skipped.
 */
static gchar *skip_tabs(gchar *str, gint count)
{
    while(*str && count) {
        while(*str && (*str != '\t')) {
            str ++;
        }

        /* skip the tab we are over..*/
        if(*str == '\t') str++;
        count --;
    }
    return str;
}


#ifdef ENABLE_NUT

/*****************************************************************************\
* NUT specific parser and client functions.                                   *
\*****************************************************************************/ 

/** Sends a command string to the socket. Hopefully there is a nut server on
 *  the other end of the socket, if so this will take the reply, null
 *  terminate it and return the number of bytes read.
 *
 *  \par Arguments:
 *  \arg \c command - The command to send to the server.
 *  \arg \c variable - The variable to include in the command string (for REQ).
 *                     Set to NULL if the command needs no variable (LISTVARS).
 *  \arg \c buffer - the buffer used to construct the command and store the result.
 *  \arg \c bufferlen - length of the buffer.
 */
static gint send_nut_command(gchar *command, gchar *variable, gchar *buffer, gint bufferlen)
{
    gint size;

    /* send the request */
    if(variable) {
        size = g_snprintf(buffer, bufferlen, "%s %s\r\n", command, variable);
    } else {
        size = g_snprintf(buffer, bufferlen, "%s\r\n", command);
    }
    write(ups_status.ups_Socket, buffer, size);
    
    /* check before we go into a block... */
    if(haltThread) return 0;

    /* FIXME: this is not guaranteed to fetch a line properly in all situations. */
    size = read(ups_status.ups_Socket, buffer, bufferlen);
    buffer[size] = '\0';
    
    return size;
}


/** Request a number from the server. Really a wrapper to send_nut_command that
 *  does some post-processing of the reply to deal with errors and convert the
 *  number at the end of the reply string into a float.
 */
static gint request_nut_float(gchar *variable, float *result, gchar *buffer, gint bufferlen)
{
    gint size;
    gchar *value;

    size = send_nut_command("REQ", variable, buffer, bufferlen);
    
    if(size > -1) {
        /* not an error? */
        if(strncmp(buffer, "ERR", 3)) {
            /* value is at "ANS "<variable>" " */
            value = buffer + 5 + strlen(variable);
            *result = (float)strtod(value, NULL);
        }
        return size;
    }

    return -1;
}


/** Request a the current status of the UPS. Does post-processing of a REQ STATUS
 *  sent to NUT via send_nut_command to replace the short status string returned
 *  by NUT with a sensible human-readable equivalent.
 */
static gint request_nut_status(gchar *buffer, gint bufferlen)
{
    gint size;
    gint row  = 0;
    gchar *result;

    size = send_nut_command("REQ", "STATUS", buffer, bufferlen);

    if(size > -1) {
        if(strncmp(buffer, "ERR", 3)) { 
            /* result is after "ANS STATUS " */
            result = buffer + 11;

            /* compare with contents of status_texts[] to obtain a human-readable form ... */
            while(status_texts[row].name) {
                if(!strncmp(result, status_texts[row].name, strlen(status_texts[row].name))) {
                    /* set status... */
                    set_last_log(&ups_status, status_texts[row].log);
                    ups_status.ups_Present = TRUE;
                    return size;
                }
                ++row;
            }
            
        }
        /* error or unknown status.. assume something bad has happened... */
        set_last_log(&ups_status, noUPS);
        ups_status.ups_Present = FALSE;
        return size;
    }

    return -1;
}


/** Read data from a NUT server and place the data in the ups_status structure.
 *  This client asks the NUT server for a list of supported variables and then enters a loop 
 *  reading each one of the available arguments in turn using the request_nut_* functions.
 *
 *  \par Arguments:
 *  \arg \c acc - buffer to use as an accumulator, must be at least MAX_LINESIZE characters in length.
 *  \arg \c temp - buffer to use as temporary store, again must be MAX_LINESIZE or greater.
 *  \return 0 on clean exit, 1 on forced, -1 if connection lost.
 */
static int ups_client_nut(gchar *acc, gchar *temp)
{
    gint size, readerr = 0;
    gint variable;

    size = send_nut_command("LISTVARS", NULL, temp, MAX_LINESIZE);

    if(size) {
        /* pase the variable list for the features we have... */
        variable = 0;
        while(ups_vars[variable].name) {
            ups_vars[variable].avail = (strstr(temp, ups_vars[variable].name) != NULL);
            ++variable;
        }
    
        /* stop now? */
        while(!haltThread && (readerr >= 0)) {
            sleep(1);
            g_mutex_lock(ups_status_lock);

            variable = 0;
            readerr  = 0;
            /* process all available float type variables */
            while(!haltThread && ups_vars[variable].name && (readerr >= 0)) {
                if(ups_vars[variable].avail && ups_vars[variable].var) {
                    readerr = request_nut_float(ups_vars[variable].name, ups_vars[variable].var, temp, MAX_LINESIZE);
                }
                ++variable;
            }

            /* status has to be handled specially... */
            if(ups_vars[VAR_STATUS].avail) {
               readerr = request_nut_status(temp, MAX_LINESIZE);
            }
            g_mutex_unlock(ups_status_lock);            
        } /* while(!haltThread) */
    }
     
    if(haltThread) return 1;

    return 0;
}

#endif /* #ifdef ENABLE_NUT */


/*****************************************************************************\
* Senty Bulldog specific parser and client functions.                         *
\*****************************************************************************/ 

/** Parse a string for UPS status data.
 *  This parses a string containing the status information generated
 *  by the upsd service into the supplied UPSData structure.
 *
 *  \note This function does not do any real checking on the string,
 *  it assumes it is a VAL string - do not expect it to parse any 
 *  other string without falling over or making a complete arse of it.
 *
 *  \par Arguments:
 *  \arg \c buffer - String to parse.
 *  \arg \c target - UPSData structure to fill in with parsed values.
 */
static void parse_VAL(gchar *buffer, struct UPSData *target)
{
    buffer = skip_tabs(buffer, 5);
    target -> bat_Voltage = (float)strtol(buffer, NULL, 10) / 10.0;
    buffer = skip_tabs(buffer, 2);
    target -> bat_Level = (float)strtol(buffer, NULL, 10) / 10.0;
    buffer = skip_tabs(buffer, 1);
    target -> in_Freq = (float)strtol(buffer, NULL, 10) / 10.0;
    buffer = skip_tabs(buffer, 1);
    target -> in_Voltage = (float)strtol(buffer, NULL, 10) / 10.0;
    buffer = skip_tabs(buffer, 8);
    target -> out_Freq = (float)strtol(buffer, NULL, 10) / 10.0;
    buffer = skip_tabs(buffer, 1);
    target -> out_Voltage = (float)strtol(buffer, NULL, 10) / 10.0;
    buffer = skip_tabs(buffer, 2);
    target -> ups_Load = (float)strtol(buffer, NULL, 10) / 10.0;
    buffer = skip_tabs(buffer,14);
    target -> ups_Temp = (float)strtol(buffer, NULL, 10) / 10.0;
           
    if(strlen(target -> ups_LastLog) == 0) {
        set_last_log(target, gotUPS);
    }
    target -> ups_Present = TRUE;
}


/** Parse a string for UPS log messages.
 *  This attempts to copy a UPS log message from the specified string into the
 *  ups_LastLog field of the supplied UPSData structure. This uses strncpy to
 *  ensure that the log buffer can not overflow.
 *
 *  \note This function can handle being called on a string even when it 
 *  doesn't contian log data, in this case the ups_LastLog of the specified
 *  UPSData structure is unchanged. This is a waste of CPU time though so
 *  it's better to check the LOG id is in the string before calling this!
 *
 *  \par Arguments:
 *  \arg \c buffer - The string to parse.
 *  \arg \c target - The UPSData structure containing the ups_LastLog to update.
 */
static void parse_LOG(gchar *buffer, struct UPSData *target)
{
    int position = 0;

    /* run along the string until we hit the start of the log text */
    while(*buffer && (*buffer != '<')) ++buffer;

    /* foound a log entry before hitting the end of string? */
    if(*buffer == '<') {
        ++buffer;

        /* copy characters until end of string, end of log or overflow */
        while(*buffer && (*buffer != '>') && (position < (MAX_LOGSIZE - 1))) {
            target -> ups_LastLog[position] = *buffer;
            ++buffer;
            ++position;
        }
        target -> ups_LastLog[position] = '\0';
    }
}


/** Parse a string for UPS information.
 *  This attempts to identify the type of information held in the specified
 *  string and uses one of the above parseXXX functions to extract useful
 *  data from the string and store it in the provided UPSData structure.
 *
 *  \par Arguments:
 *  \arg \c buffer - The string to parse. This must start "DeltaUPS:" followed
 *       by one of the codes discussed in the file comment otherwise it is 
 *       ignored and target is unchanged.
 *  \arg \c target - The UPSData structure into which the results are saved.
 *  
 *  \returns 0 if the string was not a valid or parseable UPS information 
 *  string, 1 if UPSData has been modified.
 */ 
static int parse_DeltaUPS(gchar *buffer, struct UPSData *target)
{
    /* Bad buffer contents, ignore it. This Can't Happen! 
     */
    if(strncmp(buffer, "DeltaUPS:", 9)) {
        fprintf(stderr, "parse_DeltaUPS: buffer is not a valid DeltaUPS string?! Can't Happen!\n");
        return 0;
    }

    buffer += 9;

    if(!strncmp(buffer, "VAL00", 5)) {
        buffer += 14; /* Skip VAL00,00,xxxx */
        if(*buffer != '1') {
            /* UPS has been disconnected/failed - clear structure and set error in log */
            reset_status(target);

            /* only change if status was okay */
            if(!strcmp(gotUPS, target -> ups_LastLog)) {
                set_last_log(target, noUPS);
            }
         } else {
            /* UPS is present and working, parse values */
            parse_VAL(buffer, target);
         }            
    } else if(!strncmp(buffer, "LOG00", 5)) {
        /* LOG entry.. we are only interested in the log text... */
        parse_LOG(buffer, target);
    } else {
        /* Something else we Know Not Of */
        return 0;
    }
        
    return 1;
}


/** Read data from the belkin upsd server and parse it into ups_status.
 *  The actual client work is done by this routine - it reads from the server into a temporary
 *  buffer, then the buffer is copied into an accumulator buffer until a new DeltaUPS is 
 *  encountered in the temp buffer. At this point, parseDeltaUPS() is called to parse the
 *  accumulator, then the accululator write position is reset to the start of the accumulator
 *  buffer and the copy from the temporary buffer continues.
 *
 *  \par Arguments:
 *  \arg \c acc - buffer to use as an accumulator, must be at least MAX_LINESIZE characters in length.
 *  \arg \c temp - buffer to use as temporary store, again must be MAX_LINESIZE or greater.
 *  \return 0 on clean exit, 1 on forced, -1 if connection lost.
 */
static int ups_client_belkin(gchar *acc, gchar *temp)
{
    gint readlen = 0;
    gint accpos = 0;
    gint readpos;
    
    /* continue reading from the server until we are told to stop or the server shuts down.
     * (aside: This line was a bit of a problem - in testing the read() saturates the buffer
     * for the first few calls, then the process settles down to returning 214 characters
     * for each call to read() every second. This is fine, but if read() blocks until the 
     * buffer is full on some machines then it could take MAX_LINELEN\214 seconds for the
     * read to return - my temporary solution is to only use MAX_ENTRYSIZE bytes of temp
     * with MAX_ENTRYSIZE set to 214. It's an ugly hack, but it seems to work.)
     */
    while(!haltThread && ((readlen = read(ups_status.ups_Socket, temp, MAX_ENTRYSIZE)) > 0)) {
        /* haltThread may have been set while blocked in the read, this is our Emergency Exit check.. */ 
        if(haltThread) return 1;

        temp[readlen] = 0;

        /* copy from the read buffer into the accumulator, parsing once a full record has been created */
        for(readpos = 0; readpos < readlen; ++readpos, ++accpos) {
            if(!strncmp(&temp[readpos], "DeltaUPS:", 9) || (accpos == (MAX_LINESIZE - 1))) {
                /* null terminate the accumulator buffer and reset the write position */
                acc[accpos] = 0;
                accpos = 0;

                /* convert the accumulator into easy to use stats, must be done inside lock */
                g_mutex_lock(ups_status_lock);
                parse_DeltaUPS(acc, &ups_status);
                g_mutex_unlock(ups_status_lock);
            }
            acc[accpos] = temp[readpos];
        }
    }

    /* nothing read (lost connection/error) reset status */
    if(readlen == 0) {
        readlen = -1;
        g_mutex_lock(ups_status_lock);
        reset_status(&ups_status);
        set_last_log(&ups_status, connLost);
        g_mutex_unlock(ups_status_lock);
    }        

    return readlen;
}


/*****************************************************************************\
* Top level client code and thread entrypoint.                                *
\*****************************************************************************/ 

/** Set up the connection to the upsd service and start the client routine.
 *  This obtains the address of the host running the upsd service (normally
 *  localhost, but in theory you could remotely monitor your ups from another
 *  machine with this..), attempts to conenct to it and if it manages it the
 *  upsClient() routine is used to read into the ups_status structure.
 *
 *  \todo The host lookup, connection and other tcp/ip code is IPv4 dependant.
 *  While this is not a great problem now, it may be at some point in the 
 *  future (although right now IPv6 takeup seems to be painfully slow so it
 *  is likely to be quite some time!). Really the whole client should be
 *  protocol independant.
 *
 *  \arg \c mode  is 0 or 1 for belkin, 2 for NUT 
 *  \return -2 on error, -1 on lost connection, 0 on success.
 */
static int ups_connect(gchar *hostname, guint port, guint mode)
{
    struct sockaddr_in  servaddr; /* needed for connect */
    struct hostent     *host;
    struct in_addr    **addrPtr;
    gchar *accumulator;  /* This contains the string in progress.. */
    gchar *tempStore;    /* This is filled by read() */
    gint   result = 0;
   
    fprintf(stderr, "ups_connect: connecting to %s, port %d\n", hostname, port);
    reset_status(&ups_status);

    /* Yeah, this will cause all manner of fun if it picks up an IPv6 record,
     * but for now it'll do.. */
    if((host = gethostbyname(hostname)) == NULL) {
        set_last_log(&ups_status, badHost);
        return -2;
    }

    /* Allocate the main accumulator string, Bomb if this fails.. */
    if((accumulator = (gchar *)malloc(MAX_LINESIZE)) == NULL) {
        set_last_log(&ups_status, noMem);
        return -2;
    }

    /* Likewise for the temporary buffer */
    if((tempStore = (gchar *)malloc(MAX_LINESIZE)) == NULL) {
        set_last_log(&ups_status, noMem);
        free(accumulator);
        return -2;
    }

    /* Attempt to connect to the service designated by port on the hosts obtained by the
     * call to gethostbyname(). This breaks as soon as the first successful connection is
     * established.
     */
    addrPtr = (struct in_addr **)host -> h_addr_list;
    for(; *addrPtr != NULL; ++addrPtr) {
        if((ups_status.ups_Socket = socket(AF_INET, SOCK_STREAM, 0))) {
            bzero(&servaddr, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(port);
            memcpy(&servaddr.sin_addr, *addrPtr, sizeof(struct in_addr));

            if(connect(ups_status.ups_Socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) == 0)
                break;

            close(ups_status.ups_Socket);
            ups_status.ups_Socket = 0;
        }
    }

    /* Deligate the actual communication work to the client code if a connection has
     * been established. 
     */
    if(*addrPtr != NULL) {
        if((mode == 0) || (mode == 1)) {
            result = ups_client_belkin(accumulator, tempStore);
#ifdef ENABLE_NUT
        } else {
            result = ups_client_nut(accumulator, tempStore);
#endif
        }
    } else {
        ups_status.ups_Socket = 0;
        g_mutex_lock(ups_status_lock);
        set_last_log(&ups_status, badConn);
        g_mutex_unlock(ups_status_lock);
    }

    if(ups_status.ups_Socket) {
        close(ups_status.ups_Socket);
        ups_status.ups_Socket = 0;
    }

    free(accumulator);
    free(tempStore);

    return(result);
}


/** ups client thread entrypoint.
 *  The launchClient() function uses this as the start routine argument to a
 *  g_thread_create() call. This is simply a wrapper for the upsdConnect()
 *  function which fills in the host and port from global variables.
 */
gpointer ups_start(gpointer arg)
{
    int result = 0;
    int port;

    /* attempt to reconnect after lost connections */
    while(!haltThread && (result != -2)) {
        result = ups_connect(ups_host, ups_port, ups_mode);

        if(ups_mode == 2) {
            port = process_pronet(pro_net_loc);
            if(port) {
                ups_port = port;
            }
        }
        fprintf(stderr, "ups_start: failed. Sleeping\n");
        if(!haltThread) sleep(5);
    }

    fprintf(stderr, "ups_start: exiting\n");
    return NULL;
}


/*****************************************************************************\
* thread creation and shutdown functions.                                     *
\*****************************************************************************/ 

/** Create the client thread and return the thread id.
 *  This creates a new client which connects to hostname and port. Directly
 *  creating the thread using upsStart() is fine if the host and port are
 *  unchanged from the last call to launchClient(), but in general this 
 *  should always be used when creating clients.
 */
GThread *launch_client(BUPSConfig *config)
{
    gkrellm_dup_string(&pro_net_loc, config -> pro_net);

    switch(config -> mode) {
        case 0: gkrellm_dup_string(&ups_host, "localhost");
                ups_port = process_pronet(pro_net_loc);
                break;
        case 1: gkrellm_dup_string(&ups_host, config -> belkin_host);
                ups_port = config -> belkin_port;
                break;
        case 2: gkrellm_dup_string(&ups_host, config -> nut_host);
                ups_port = config -> nut_port;
                break;
    }

    ups_mode = config -> mode;

    /* make sure threads are enabled... */
    if(!g_thread_supported()) g_thread_init(NULL);
    if(ups_status_lock == NULL) ups_status_lock = g_mutex_new();

    return g_thread_create(ups_start, NULL, TRUE, NULL);
}


/** Force the specified client thread to exit.
 *  This will tell the client thread(s) to halt and wait for thread 'tid' to  
 *  exit before continuting. Use sparingly or it may affect gkrellm updates!
 */
void halt_client(GThread *tid)
{
    haltThread = TRUE;

    /* got to be a better way to take the client out ... :/ */
    g_thread_join(tid);
    haltThread = FALSE;

    if(pro_net_loc) {
        g_free(pro_net_loc);
        pro_net_loc = NULL;
    }    

    if(ups_host) {
        g_free(ups_host);
        ups_host = NULL;
    }            
}
    
