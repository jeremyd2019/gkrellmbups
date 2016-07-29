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
 *  \file config.h
 *  Contains prototypes for functions exported by prefs.c and various default
 *  definitions.
 */
/*  $Id: prefs.h,v 1.2 2003/02/06 21:07:53 chris Exp $
 */

#ifndef _CONFIG_H
#define _CONFIG_H 1

#ifndef GKRELLM_VERSION_MAJOR
    #include<gkrellm2/gkrellm.h>
#endif

#include<glib.h>
#include<gtk/gtk.h>

/*! Value to subtract from input and output mains voltages.
 *  Mains voltages are typically 220 to 240 in the UK, this presented some problems with
 *  fitting any sort of meaningful data on the chart - either the chart is prohibitively
 *  large or any difference between the signals is swamped by the scale. Subtracting 
 *  MAINS_MIN from the values before display ensures that both signals will fit on the 
 *  display without significant loss of detail on a 40/50 pixel high monitor.
 *  \note MAINS_MIN should be approximately the same as the UPS low voltage transfer
 *  value (187 on a UK Belkin UPS)
 */
#ifndef MAINS_MIN
#define MAINS_MIN               190           
#endif

#define MODE_LOCAL              0             /*!< Monitor a local Belkin UPS (Sentry Bulldog)      */
#define MODE_REMOTE             1             /*!< Monitor a remote Belkin UPS (Sentry Bulldog)     */
#define MODE_NUT                2             /*!< Monitor a UPS via NUT                            */

#define DEFAULT_MODE            MODE_LOCAL    /*!< Default to local Belkin monitoring               */
#define DEFAULT_PRONET          "/usr/local/bulldog/PRO_NET.DAT"  /*<! Default location of UPS data */
#define DEFAULT_BELKIN_HOST     "localhost"   /*!< address of the computer on which upsd is running */
#define DEFAULT_BELKIN_PORT     2710          /*!< port upsd is accepting connections on            */
#define DEFAULT_NUT_HOST        "localhost"   /*!< Address of the computer that NUT is on           */
#define DEFAULT_NUT_PORT        3493          /*!< Official IANA NUT port.                          */
#define DEFAULT_NUT_AUTH        0             /*!< Enable authorisation stuff. Default is no (0)    */

#define DEFAULT_VFORMAT "i:\\f$i,\\.o:\\f$o,\\nb:\\f$l%" /*<! Default voltage chart format.         */
#define DEFAULT_FFORMAT "i:\\f$i\\no:\\f$o"              /*<! Default frequency chart format.       */ 
#define DEFAULT_TFORMAT "t:\\f$tC\\nl:\\f$l%"            /*<! Default temperature chart format.     */

/*! Size of the buffers used for storing configuration data in loadConfig().                        */
#define CONFIG_BUFSIZE 256         

/*! Configuration option storage.
 *  The non-chart-specific options are stored in this structure. Information specific to charts
 *  (text formats etc) is stored in the chart structure. 
 */
typedef struct
{
    gint         mode;                       /*!< Plugin operation mode                                                     */
    gchar       *pro_net;                    /*!< Location of bulldog's PRO_NET.DAT file                                    */
    gchar       *belkin_host;                /*!< Hostname on which the upsd service is running (default is localhost).     */
    gint         belkin_port;                /*!< Port on which the upsd server is accepting connections (2710 is default). */
    gchar       *nut_host;                   /*!< Hostname on which NUT is running (default is localhost).                  */
    gint         nut_port;                   /*!< Port NUT is listening on (3492 is the IANA allocation)                    */
    gint         nut_auth;                   /*!< Authenticate nut connection?                                              */
    gchar       *nut_username;               /*!< Username to pass to NUT                                                   */
    gchar       *nut_password;               /*!< Password to pass to NUT                                                   */
    gint         show_log;                   /*!< 0 to show label, 1 to show log.                                           */
    gint         mains;                      /*!< Utility low battery transfer voltage or similar.                          */ 
    gboolean     show_volt;                  /*!< Show the voltages? Defaults to TRUE.                                      */
    gboolean     show_freq;                  /*!< Show the frequencies? Defaults to TRUE.                                   */
    gboolean     show_stat;                  /*!< Show the statistics? Defaults to TRUE.                                    */
    gboolean     show_msgs;                  /*!< Show the log message bar? Defaults to TRUE.                               */
} BUPSConfig;

extern void        bups_create_gui   (GtkWidget *tab);
extern BUPSConfig *bups_create_config(void);
extern void        bups_save_config  (FILE *file);
extern void        bups_load_config  (gchar *line);
extern void        bups_apply_config (void);

#endif /* #ifndef _CONFIG_H */
