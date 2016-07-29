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
 *  \file gkrellmbups.h
 *  Header for the GKrellM plugin.
 *  This header is mainly here to pull all the structure definitions and #defines
 *  from various other files together in one place and to provide access to the
 *  "core" GKrellMBUPS structure used in various places throughout the plugin.
 */
/*  $Id: gkrellmbups.h,v 1.2 2003/02/06 21:07:53 chris Exp $
 */

#ifndef _GKRELLMBUPS_H
#define _GKRELLMBUPS_H 1

#ifndef GKRELLM_VERSION_MAJOR
    #include<gkrellm2/gkrellm.h>
#endif

#include<glib.h>
#include"chart.h"
#include"prefs.h"
#include"../config.h"

#define CONFIG_NAME             "GKrellMBUPS"  /*!< Name for the configuration tab.    */
#define MONITOR_CONFIG_KEYWORD  "gkrellmbups"  /*!< Configuration name.                */
#define STYLE_NAME              "gkrellmbups"  /*!< style name to allow custom themes. */
#define INSERT_BEFORE           MON_FS         /*!< Insert plugin before this monitor. */

/*! Central data store structure.
 *  This contains pointers to all the UPSChart structures and various gkrellm 
 *  objects used by the plugin.
 */
typedef struct
{
    BUPSConfig   *config;       /*!< Configuration data.                                         */
    BUPSChart     volt_chart;   /*!< Input and output and battery voltage display.               */
    BUPSChart     freq_chart;   /*!< Input and output frequency chart.                           */
    BUPSChart     temp_chart;   /*!< Temperature and load chart (fixed max is 100).              */
    GkrellmPanel *log_display;  /*!< Panel on which a decal can scroll the last UPS log message. */
    GkrellmStyle *log_style;    /*!< Style data for the loag display panel.                      */
    GkrellmDecal *log_decal;    /*!< Decal used on logDisplay.                                   */
    gchar        *log_label;    /*!< Text displayed when the log display is deactivated          */
    gchar        *log_text;     /*!< Text displayed when the log display is activated            */
    gint          log_scr;      /*!< Horizontal scroll                                           */
    GkrellmDecal *label_decal;  /*!< Decal used on logDisplay.                                   */
    gint          label_x;      /*!< Horizontal position of the label                            */
    GtkWidget    *vbox;
    GThread      *client;       /*!< FIXME: 1.0 safe only, fix for 2.0                           */
} GKrellMBUPS;

extern GKrellMBUPS    *bups_data;
extern GkrellmMonitor *bups_mon;
extern gint            bups_style_id;

#endif /* _GKRELLMBUPS_H */
