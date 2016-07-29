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
 *  \file chart.h
 *  This file contains the functions exported by chart.c and the BUPSChart
 *  structure that has to be visible to ups_connect.h and prefs.c.
 */
/*  $Id: chart.h,v 1.2 2003/02/06 21:07:53 chris Exp $
 */

#ifndef _CHART_H
#define _CHART_H 1

#ifndef GKRELLM_VERSION_MAJOR
    #include<gkrellm2/gkrellm.h>
#endif

#include<glib.h>

#define MAX_DATA  3 /*!< Maximum number of chartdata entries per chart */

#define DRAW_BUFFER_SIZE     64             /*!< length of temporary store buffer for the drawing code.    */

/*! Structure containing data related to a single chart object.
 *  This structure contains pointers to the various elements which together form
 *  a single chart in the GKrellM window (chart, config, panel etc).
 */
typedef struct
{
    GtkWidget          *vbox;           /*!< Box into which the Chart and then Panel are added. */
    GkrellmChart       *chart;          /*!< The chart contained in vbox. */
    GkrellmChartdata   *data[MAX_DATA]; /*!< The data shown in the chart. */
    GkrellmChartconfig *config;         /*!< Settings structure for the chart. */
    GkrellmStyle       *style;          /*!< chart and panel style. */
    GkrellmPanel       *panel;          /*!< The panel shown beneath the chart, this is just a label really. */
    gboolean            show_text;      /*!< True if the chart text overlay should be drawn. */
    char               *text_format;    /*!< Text overlay format for this chart. */
    gchar               draw_buffer[DRAW_BUFFER_SIZE];
    void              (*format)(gchar *, gint, gchar *); /*!< Text formatting function */
} BUPSChart;


extern void bups_create_plugin(GtkWidget *vbox, gint firstCreate);
extern void bups_update_plugin(void);

#endif /* #ifndef _CHART_H */
