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
 *  \file ups_connect.h
 *  UPS service connection and parsing header.
 *  This file is the header for the ups_connect.c file, it contains the 
 *  structures visible to other modules along with some other bits and pieces..
 *
 */
/*  $Id: ups_connect.h,v 1.2 2003/02/06 21:07:53 chris Exp $
 */

#ifndef UPS_CONNECT
#define UPS_CONNECT 1

#include<glib.h>
#include"prefs.h"

/*! Please keep logs under this size - I enforce it anyway...                             */
#define MAX_LOGSIZE 256 

/*! Maximum size of a single DeltaUPS line (the largest I've found is around 350 chars)   */
#define MAX_LINESIZE 1024

/*! Maximum size of a single entry (see the aside in upsClient() for more on this #define */
#define MAX_ENTRYSIZE 213

/*! Maximum length of a line in PRO_NET.DAT.                                              */
#define MAX_PRONET   512

/** Structure to store UPS status values.
 *  This contains all the values I have been able to reverse engineer from the
 *  upsd output. The ups connect code attemps to parse the output of upsd into
 *  the fields of an instance of this structure. 
 */
struct UPSData
{
    gfloat   bat_Voltage;              /*!< Current pattery power in volts. */
    gfloat   bat_Level;                /*!< Battery power as a percentage of maximum power. */
    gfloat   in_Freq;                  /*!< Frequency of the utility input. */
    gfloat   in_Voltage;               /*!< Utility input voltage (always around 250v in my house!) */
    gfloat   out_Freq;                 /*!< Frequency the UPS is chucking out. */
    gfloat   out_Voltage;              /*!< Voltage the UPS is supplying (usually 220v for me). */
    gfloat   ups_Load;                 /*!< Connected load value */
    gfloat   ups_Temp;                 /*!< Internal temperature. */
    gchar    ups_LastLog[MAX_LOGSIZE]; /*!< Last log message (or error message from us...) */
    gboolean ups_Present;              /*!< TRUE if UPS connected, FALSE otherwise.  */
    int      ups_Socket;               /*!< Socket which is connected to the upsd service. */
};

/* global variables exported from ups_connect.c 
 *
 */
extern struct UPSData  ups_status;      /*!< Global UPS data structure, must be synchronised across threads! */
extern GMutex    *ups_status_lock; /*!< Synchronisation mutex for upsStatus.                            */

/* functions exported from ups_connect.c */
extern GThread* launch_client(BUPSConfig *config); /*!< Create the client thread and return the thread id. */
extern void     halt_client  (GThread* tid);      /*!< Force the specified client thread to exit.         */ 

#endif
