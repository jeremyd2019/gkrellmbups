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
 *  \file gkrellmbups.c
 *  Plugin entry function and GKrellM Monitor structure setup. All the actual
 *  work done by the plugin is in chart.c, prefs.c and ups_connect.c
 */
/*  $Id: gkrellmbups.c,v 1.2 2003/02/06 21:07:53 chris Exp $
 */

#include "gkrellmbups.h"

GKrellMBUPS    *bups_data;
GkrellmMonitor *bups_mon;
gint            bups_style_id;

/** GKrellM Monitor structure for this plugin. 
 *  A pointer to this is returned by init_plugin.
 */
/* FIXME: 1.0 safe only, update structures for 2.0 
 */
static GkrellmMonitor mon =
{
    CONFIG_NAME,            /*!< Name, for config tab.                   */
    0,                      /*!< Id,  0 if a plugin                      */
    bups_create_plugin,     /*!< The create_plugin() function            *//* in chart.c  */
    bups_update_plugin,     /*!< The update_plugin() function            *//* in chart.c  */
    bups_create_gui,        /*!< The create_plugin_tab() config function *//* in config.c */
    bups_apply_config,      /*!< The apply_plugin_config() function      *//* in config.c */

    bups_save_config,       /*!< The save_plugin_config() function       *//* in config.c */
    bups_load_config,       /*!< The load_plugin_config() function       *//* in config.c */
    MONITOR_CONFIG_KEYWORD, /*!< config keyword                          */

    NULL,                   /*!< Undefined 2  */
    NULL,                   /*!< Undefined 1  */
    NULL,                   /*!< private      */

    INSERT_BEFORE,          /*!< Insert plugin before this monitor.      */
    NULL,                   /*!< Handle if a plugin filled in by GKrellM */
    NULL                    /*!< path if a plugin filled in by GKrellM   */
};

/** Plugin initialisation function. 
 */
GkrellmMonitor *gkrellm_init_plugin(void)
{
    bups_data = g_new0(GKrellMBUPS, 1);
    bups_data -> config = bups_create_config();

	bups_style_id = gkrellm_add_chart_style(&mon, STYLE_NAME);
	bups_mon = &mon;

    fprintf(stderr, "gkrellm_init_plugin: initalising 2.0.2\n");

	return bups_mon;
}
