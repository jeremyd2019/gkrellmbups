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
 *  \file config.c
 *  All the configuration GUI setup and config data loading and saving is done
 *  in this file.
 */
/*  NB: This file contains several block-commented code sections corresponding 
 *  to authentication facilities for NUT connections. While the code is in a
 *  usable state, I'm not sure it is needed so it is left out for the moment. 
 *  If you need the NUT authentication facilities, please tell me and I will 
 *  make the appropriate changes to enable to auth options.
 */
/*  $Id: prefs.c,v 1.2 2003/02/06 21:07:53 chris Exp $
 */

#include<gtk/gtk.h>
#include"../config.h"
#include"version.h"
#include"gkrellmbups.h"
#include"ups_connect.h"

/* Some macros to make table additions neater */
#define GTK_TABLE_DEFX ((GtkAttachOptions)(GTK_EXPAND | GTK_FILL))
#define GTK_TABLE_DEFY ((GtkAttachOptions)(0))

/* Size of the buffer used when constructing the mains combo */
#define MAINS_BUF_SIZE 32


/*****************************************************************************\
* Texts and option lists.                                                     *
\*****************************************************************************/ 
 
/*! Descriptive text shown in the Help tab of the plugin configuration. */ 
static gchar *help_text[] = 
{
    "GKrellMBUPS displays the status of a UPS by interrogating either Belkin \"Sentry\n",
    "Bulldog\" software (for Belkin UPSs) or Network UPS Tools (any UPS supported by\n",
    "NUT). Monitoring of local or remote UPSs is supported. Note that neither Belkin\n",
    "Sentry Bulldog or NUT are not supplied with this plugin and you must have one\n",
    "of these packages installed before GKrellMBUPS can be used (see the README!)\n",
    "\n",
    "<b>\"Mains\" setting:\n",
    "Mains voltages are too high to show on a reasonable size chart while retaining any \n",
    "detail. This option allows you to set the value to subtract from the mains voltage \n",
    "before showing it on the chart - this makes changes in voltage far more visible. It \n",
    "should be set to approximately the voltage at which your UPS switches to battery \n",
    "supply (around 187v on a UK UPS).\n\n",
    "<b>Voltage chart:\n",
    "Substitution variables for the format string for chart labels:\n",
    "\t$i\tInput voltage level (in volts)\n", 
    "\t$o\tOutput voltage level (in volts)\n", 
    "\t$b\tBattery voltage level (in volts)\n", 
    "\t$l\tBattery level\n", 
    "\n",
    "<b>Frequency chart:\n",
    "Substitution variables for the format string for chart labels:\n",
    "\t$i\tInput frequency (in hertz)\n", 
    "\t$o\tOutput frequency (in hertz)\n", 
    "\n",
    "<b>Stats chart:\n",
    "Substitution variables for the format string for chart labels:\n",
    "\t$t\tUPS Temperature (in centigrade)\n", 
    "\t$l\tLoad level (as a percentage of maximum)\n", 
    "\n",
    "Left click on charts to toggle the text overlay. Middle click on the UPS panel to\n",
    "toggle a scrolling display of log messages from the UPS."
};

/*! Plugin ownership and version information show in the About table of the plugin configuration. */
static gchar about_text[] = "GKrellMBUPS %d.%d.%d\nGKrellM Belkin UPS Monitor plugin\n\n\nCopyright (C) 2001-2002 Chris Page\nChris <chris@starforge.co.uk>\nhttp://www.starforge.co.uk/gkrellm/\n\nReleased under the GPL.\n";

/*! Options listed in the voltage chart format popup */
static gchar *voltage_options[] = 
{
    "i:\\f$i,\\.o:\\f$o,\\nb:\\f$b",
    "i:\\f$i,\\.o:\\f$o,\\nb:\\f$l%",
    "i:\\f$i,\\.o:\\f$o",
    "i:\\f$i\\no:\\f$o",
    NULL
};

/*! Options listed in the frequency chart format popup */
static gchar *frequency_options[] = 
{
    "i:\\f$i\\no:\\f$o",
    "i:\\f$i,\\.o:\\f$o",
    NULL
};

/*! Options listed in the temperature/load chart format popup */
static gchar *temperature_options[] =
{
    "t:\\f$tC\\nl:\\f$l%",
    "t:\\f$tC,\\.l:\\f$l%",
    NULL
};

/*! Options listed in the mains popup */
static gchar *mains_options[] =
{
    "210",
    "180",
    "110",
    "90",
    NULL
};

/*! global file requester (used by PRO_NET.DAT location code) */ 
static GtkWidget *file_selector;

/*! global widget pointers */
static GtkWidget *voltage_combo;
static GtkWidget *frequency_combo;
static GtkWidget *temperature_combo;
static GtkWidget *mains_combo;
static GtkWidget *client_mode;
static GtkWidget *mode[4];
static GtkWidget *mode_options;
static GtkWidget *pronet_location;
static GtkWidget *remote_host;
static GtkWidget *remote_port;
static GtkWidget *show_volt;
static GtkWidget *show_freq;
static GtkWidget *show_stats;
static GtkWidget *show_msgs;

#ifdef ENABLE_NUT
static GtkWidget *nut_host;
static GtkWidget *nut_port;
#endif
/*static GtkWidget *nut_authenticate;
 *static GtkWidget *nut_username;
 *static GtkWidget *nut_password;
 */

static gint       activemode = 0;

/*****************************************************************************\
* GUI callback functions.                                                     *
\*****************************************************************************/ 

/*! Callback for the OK button in the file requester. 
 */
static void cb_file_ok(GtkWidget *widget, GtkFileSelection *fs)
{ 
    gtk_entry_set_text(GTK_ENTRY(pronet_location), 
                       gtk_file_selection_get_filename(GTK_FILE_SELECTION (fs)));
    gtk_widget_hide(GTK_WIDGET(file_selector));
}

/*! Callback which opens the file requester.
 */

static void cb_open_fileselect(GtkWidget *widget, gpointer *data)
{ 
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selector), 
                                    gtk_entry_get_text(GTK_ENTRY(pronet_location)));

    gtk_widget_show(GTK_WIDGET(file_selector));
}


/*#ifdef ENABLE_NUT
static void cb_auth_toggle(GtkWidget *widget, gpointer data)
{
    gboolean sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    gtk_widget_set_sensitive(nut_username, sensitive);
    gtk_widget_set_sensitive(nut_password, sensitive);
}
#endif*/


static void cb_mode_change(GtkWidget *widget, gpointer data)
{
    gint       option;

#if (GKRELLMBUPS_VERSION_MAJOR == 1)
    GtkWidget *menu;
    GtkWidget *item;

    menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(client_mode));
    item = gtk_menu_get_active(GTK_MENU(menu));

    for(option = 0; ((mode[option] != NULL) && (mode[option] != item)); ++option) {
        /* EMPTY */
    }
    if(mode[option] == NULL) {
        option = 0;
    } 

    gtk_notebook_set_page(GTK_NOTEBOOK(mode_options), option);
    activemode = option;
#else
    option = gtk_option_menu_get_history(GTK_OPTION_MENU(client_mode));
    gtk_notebook_set_current_page(GTK_NOTEBOOK(mode_options),  option);
    activemode = option;
#endif
}


/*****************************************************************************\
* GUI creation functions.                                                     *
\*****************************************************************************/ 


static GtkWidget *create_fileselect(void)
{
    file_selector = gtk_file_selection_new("Locate PRO_NET.DAT");

#if (GKRELLMBUPS_VERSION_MAJOR == 1)
    /* All destroys really just hide the file selector */
    gtk_signal_connect(GTK_OBJECT(file_selector), "destroy",
                       (GtkSignalFunc)gtk_widget_hide, file_selector);

    /* Connect the ok_button to cb_file_ok function */
    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_selector) -> ok_button), "clicked", 
                       (GtkSignalFunc)cb_file_ok, file_selector);
    
    /* Connect the cancel_button to hide the widget */
    gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(file_selector) -> cancel_button), "clicked", 
                              (GtkSignalFunc)gtk_widget_hide, GTK_OBJECT(file_selector));
#else 
    /* All destroys really just hide the file selector */
    g_signal_connect(G_OBJECT(file_selector), "destroy",
                     G_CALLBACK(gtk_widget_hide), (gpointer)file_selector);

    /* Connect the ok_button to cb_file_okl function */
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(file_selector) -> ok_button), "clicked", 
                     G_CALLBACK(cb_file_ok), (gpointer)file_selector);
    
    /* Connect the cancel_button to hide the widget */
    /* (NOTE: this is taken from the tutorial - WTF is _swapped about?!?!) */
    g_signal_connect_swapped(G_OBJECT(GTK_FILE_SELECTION(file_selector) -> cancel_button), "clicked", 
                             G_CALLBACK(gtk_widget_hide), G_OBJECT(file_selector));    
#endif

    return file_selector;
}


static GtkWidget *create_label(char *text)
{
    GtkWidget *label;

    label = gtk_label_new(text);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_widget_show(label);

    return label;
}


static GtkWidget *create_menu_item(GtkWidget *menu, char *label)
{
    GtkWidget *menu_item;
    
    menu_item = gtk_menu_item_new_with_label(label);
    gtk_widget_show(menu_item);

#if (GKRELLMBUPS_VERSION_MAJOR == 1)
    gtk_menu_append(GTK_MENU(menu), menu_item);    
#else 
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);    
#endif

    return menu_item;
}


static GtkWidget *create_combo(char **entries, char *initial)
{
    GtkWidget *combo;
    gchar     *item;
    GList     *combo_items = NULL;
    
    /* build the combo list */
    while((item = *entries++) != NULL) {
        combo_items = g_list_append(combo_items, (gpointer)item);
    }

    combo = gtk_combo_new();
 
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), combo_items);
    g_list_free(combo_items);

    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), initial);

    gtk_widget_show(GTK_COMBO(combo)->entry); /* don't know if this is needed, but it won't hurt.. */
    gtk_widget_show(combo);

    return combo;
}    


static GtkWidget *create_chart_frame(void)
{
    GtkWidget *chart_frame;
    GtkWidget *settings_table;
    GtkWidget *label;
    gchar      mains_buffer[MAINS_BUF_SIZE];
    
    g_snprintf(mains_buffer, MAINS_BUF_SIZE, "%d", bups_data -> config -> mains);

    chart_frame = gtk_frame_new("Chart settings");

    settings_table = gtk_table_new(4, 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(settings_table), 2);
    gtk_table_set_col_spacings(GTK_TABLE(settings_table), 2);
    gtk_container_add(GTK_CONTAINER(chart_frame), settings_table);

    voltage_combo     = create_combo(voltage_options, bups_data -> volt_chart.text_format);
    label             = create_label("Voltage chart format");
    gtk_table_attach(GTK_TABLE(settings_table), voltage_combo, 0, 1, 0, 1, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);
    gtk_table_attach(GTK_TABLE(settings_table), label        , 1, 2, 0, 1, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    frequency_combo   = create_combo(frequency_options, bups_data -> freq_chart.text_format);
    label             = create_label("Frequency chart format");
    gtk_table_attach(GTK_TABLE(settings_table), frequency_combo, 0, 1, 1, 2, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);
    gtk_table_attach(GTK_TABLE(settings_table), label          , 1, 2, 1, 2, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    temperature_combo = create_combo(temperature_options, bups_data -> temp_chart.text_format);
    label             = create_label("Temperature/load chart format");
    gtk_table_attach(GTK_TABLE(settings_table), temperature_combo, 0, 1, 2, 3, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);
    gtk_table_attach(GTK_TABLE(settings_table), label            , 1, 2, 2, 3, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    mains_combo       = create_combo(mains_options, mains_buffer);
    label             = create_label("Mains voltage offset");
    gtk_table_attach(GTK_TABLE(settings_table), mains_combo, 0, 1, 3, 4, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);
    gtk_table_attach(GTK_TABLE(settings_table), label      , 1, 2, 3, 4, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    gtk_widget_show(settings_table);
    gtk_widget_show(chart_frame);

    return chart_frame;
}


static void create_local_tab(GtkWidget *notebook)
{
    GtkWidget *local_options;
    GtkWidget *set_pronet;
    GtkWidget *label;

    local_options = gtk_table_new(1, 3, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(local_options), 2);
    gtk_table_set_col_spacings(GTK_TABLE(local_options), 2);
    gtk_container_add(GTK_CONTAINER(mode_options), local_options);

    pronet_location = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(pronet_location), bups_data -> config -> pro_net);
    gtk_widget_show(pronet_location);
    gtk_table_attach(GTK_TABLE(local_options), pronet_location, 0, 1, 0, 1, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    set_pronet = gtk_button_new_with_label("...");
    gtk_widget_show(set_pronet);
    gtk_table_attach(GTK_TABLE(local_options), set_pronet, 1, 2, 0, 1, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    label = create_label("PRO_NET.DAT location");
    gtk_table_attach(GTK_TABLE(local_options), label, 2, 3, 0, 1, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

#if (GKRELLMBUPS_VERSION_MAJOR == 1)
    gtk_signal_connect(GTK_OBJECT(set_pronet), "clicked", 
                       (GtkSignalFunc)cb_open_fileselect, NULL);
#else 
    g_signal_connect(G_OBJECT(set_pronet), "clicked", 
                     G_CALLBACK(cb_open_fileselect), NULL);
#endif

    gtk_widget_show(local_options);
}


static void create_remote_tab(GtkWidget *notebook)
{
    GtkWidget *remote_options;
    GtkObject *adjust;
    GtkWidget *label;

    remote_options = gtk_table_new(2, 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(remote_options), 2);
    gtk_table_set_col_spacings(GTK_TABLE(remote_options), 2);
    gtk_container_add(GTK_CONTAINER(mode_options), remote_options);

    remote_host = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(remote_host), bups_data -> config -> belkin_host);
    gtk_widget_show(remote_host);
    gtk_table_attach(GTK_TABLE(remote_options), remote_host, 0, 1, 0, 1, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    label = create_label("Hostname");
    gtk_table_attach(GTK_TABLE(remote_options), label, 1, 2, 0, 1, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    adjust = gtk_adjustment_new(bups_data -> config -> belkin_port, 0, 65535, 1, 10, 10);
    remote_port = gtk_spin_button_new(GTK_ADJUSTMENT(adjust), 1, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(remote_port), TRUE);
    gtk_widget_show(remote_port);
    gtk_table_attach(GTK_TABLE(remote_options), remote_port, 0, 1, 1, 2, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    label = create_label("Port");
    gtk_table_attach(GTK_TABLE(remote_options), label, 1, 2, 1, 2, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    gtk_widget_show(remote_options);

}

#ifdef ENABLE_NUT

static void create_nut_tab(GtkWidget *notebook)
{
    GtkWidget *nut_options;
    GtkObject *adjust;
    GtkWidget *label;

    nut_options = gtk_table_new(5, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(mode_options), nut_options);
    gtk_table_set_row_spacings(GTK_TABLE(nut_options), 2);
    gtk_table_set_col_spacings(GTK_TABLE(nut_options), 2);

    /* general fields - more or less the same as the remote tab */
    nut_host = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(nut_host), bups_data -> config -> nut_host);
    gtk_widget_show(nut_host);
    gtk_table_attach(GTK_TABLE(nut_options), nut_host, 0, 1, 0, 1, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    label = create_label("Hostname");
    gtk_table_attach(GTK_TABLE(nut_options), label   , 1, 2, 0, 1, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    adjust = gtk_adjustment_new(bups_data -> config -> nut_port, 0, 6535, 1, 10, 10);
    nut_port = gtk_spin_button_new(GTK_ADJUSTMENT(adjust), 1, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(nut_port), TRUE);
    gtk_widget_show(nut_port);
    gtk_table_attach(GTK_TABLE(nut_options), nut_port, 0, 1, 1, 2, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    label = create_label("Port");
    gtk_table_attach(GTK_TABLE(nut_options), label   , 1, 2, 1, 2, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);


    /* NUT authentication... */
    /* NOTE WELL: From NUT docs/protocol.txt: 
     * LOGIN 
     * =====
     * Form: LOGIN [<upsname>]
     * ......
     * NOTE: You probably shouldn't send this command unless you are upsmon,
     *  or a upsmon replacement.
     *
     * Not sure if this means me, so I'm playing on the safe side here.... 
     */
    /*
    nut_authenticate = gtk_check_button_new_with_label("Send username and password");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(nut_authenticate), bups_data -> config -> nut_auth);
    gtk_widget_show(nut_authenticate);
    gtk_table_attach(GTK_TABLE(nut_options), nut_authenticate, 0, 1, 2, 3, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    nut_username = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(nut_username), bups_data -> config -> nut_username);
    gtk_widget_set_sensitive(nut_username, bups_data -> config -> nut_auth);
    gtk_widget_show(nut_username);
    gtk_table_attach(GTK_TABLE(nut_options), nut_username, 0, 1, 3, 4, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    label = create_label("Username");
    gtk_table_attach(GTK_TABLE(nut_options), label       , 1, 2, 3, 4, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    nut_password = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(nut_password), FALSE);
    gtk_entry_set_text(GTK_ENTRY(nut_username), bups_data -> config -> nut_password);
    gtk_widget_set_sensitive(nut_password, bups_data -> config -> nut_auth);
    gtk_widget_show(nut_password);
    gtk_table_attach(GTK_TABLE(nut_options), nut_password, 0, 1, 4, 5, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    label = create_label("Password");
    gtk_table_attach(GTK_TABLE(nut_options), label       , 1, 2, 4, 5, GTK_TABLE_DEFX, GTK_TABLE_DEFY, 0, 0);

    #if (GKRELLMBUPS_VERSION_MAJOR == 1)
    gtk_signal_connect(GTK_OBJECT(nut_authenticate), "clicked", 
                       (GtkSignalFunc)cb_auth_toggle, NULL);
    #else 
    g_signal_connect(G_OBJECT(nut_authenticate), "clicked", 
                     G_CALLBACK(cb_auth_toggle), NULL);
    #endif

    */ 

    gtk_widget_show(nut_options);
}

#endif /* #ifdef ENABLE_NUT */


static GtkWidget *create_client_frame(void)
{
    GtkWidget *client_frame;
    GtkWidget *client_settings;
    GtkWidget *menu;
 
    client_frame = gtk_frame_new(_("UPS Server"));

    client_settings = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(client_settings);
    gtk_container_add(GTK_CONTAINER(client_frame), client_settings);

    client_mode = gtk_option_menu_new();

    menu = gtk_menu_new();
    mode[0] = create_menu_item(menu, "Local Belkin UPS (Sentry Bulldog upsd)");
    mode[1] = create_menu_item(menu, "Remote Belkin UPS (Sentry Bulldog uspd)");
#ifdef ENABLE_NUT
    mode[2] = create_menu_item(menu, "Network UPS Tools monitored UPS");
    mode[3] = NULL;
#else
    mode[2] = NULL;
#endif
    gtk_option_menu_set_menu(GTK_OPTION_MENU(client_mode), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(client_mode), bups_data -> config -> mode);
    activemode = bups_data -> config -> mode;

    gtk_widget_show(client_mode);
    gtk_box_pack_start(GTK_BOX(client_settings), client_mode, FALSE, FALSE, 0);

    mode_options = gtk_notebook_new();
    gtk_widget_show(mode_options);
    gtk_box_pack_start(GTK_BOX(client_settings), mode_options, TRUE, TRUE, 0);
    GTK_WIDGET_UNSET_FLAGS(mode_options, GTK_CAN_FOCUS);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(mode_options), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(mode_options), FALSE);

    create_local_tab(mode_options);
    create_remote_tab(mode_options);

#ifdef ENABLE_NUT
    create_nut_tab(mode_options);
#endif

#if (GKRELLMBUPS_VERSION_MAJOR == 1)
    gtk_notebook_set_page(GTK_NOTEBOOK(mode_options),  bups_data -> config -> mode);
    gtk_signal_connect(GTK_OBJECT(menu), "selection-done", 
                       (GtkSignalFunc)cb_mode_change, NULL);
#else 
    gtk_notebook_set_current_page(GTK_NOTEBOOK(mode_options),  bups_data -> config -> mode);
    g_signal_connect(G_OBJECT(client_mode), "changed", 
                     G_CALLBACK(cb_mode_change), NULL);
#endif

    gtk_widget_show(client_frame);
    return client_frame;
}


static void create_options_tab(GtkWidget *notebook)
{
    GtkWidget *options_vbox;
    GtkWidget *chart;
    GtkWidget *client;
    GtkWidget *tab_label;

    options_vbox = gtk_vbox_new (FALSE, 0);

    chart = create_chart_frame();
    gtk_box_pack_start(GTK_BOX(options_vbox), chart, TRUE, TRUE, 0);

    client = create_client_frame();    
    gtk_box_pack_start(GTK_BOX(options_vbox), client, TRUE, TRUE, 0);

    tab_label = gtk_label_new("Options");
    gtk_widget_show(tab_label);
    gtk_widget_show(options_vbox);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), options_vbox, tab_label);
}


static void create_toggles_tab(GtkWidget *notebook)
{
    GtkWidget *toggles_vbox;
    GtkWidget *toggles;
    GtkWidget *tab_label;

    toggles_vbox = gtk_vbox_new (FALSE, 0);

    toggles = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(toggles);

    show_volt = gtk_check_button_new_with_mnemonic(_("Show voltage chart"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_volt), bups_data -> config -> show_volt);
    gtk_widget_show(show_volt);
    gtk_box_pack_start(GTK_BOX(toggles), show_volt, FALSE, FALSE, 0);

    show_freq = gtk_check_button_new_with_mnemonic(_("Show frequencies chart"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_freq), bups_data -> config -> show_freq);
    gtk_widget_show(show_freq);
    gtk_box_pack_start(GTK_BOX(toggles), show_freq, FALSE, FALSE, 0);

    show_stats = gtk_check_button_new_with_mnemonic(_("Show temperature and load chart"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_stats), bups_data -> config -> show_stat);
    gtk_widget_show(show_stats);
    gtk_box_pack_start(GTK_BOX(toggles), show_stats, FALSE, FALSE, 0);

    show_msgs = gtk_check_button_new_with_mnemonic(_("Show UPS/Log panel"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_msgs), bups_data -> config -> show_msgs);
    gtk_widget_show(show_msgs);
    gtk_box_pack_start(GTK_BOX(toggles), show_msgs, FALSE, FALSE, 0);
   
    gtk_box_pack_start(GTK_BOX(toggles_vbox), toggles, TRUE, TRUE, 0);

    tab_label = gtk_label_new("Toggles");
    gtk_widget_show(tab_label);
    gtk_widget_show(toggles_vbox);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), toggles_vbox, tab_label);
}


static void create_help_tab(GtkWidget *notebook)
{
    GtkWidget *vbox;
    GtkWidget *help_area;
    gint       i;

#if (GKRELLMBUPS_VERSION_MAJOR == 1)
    vbox      = gkrellm_create_framed_tab(notebook, "Help");
	help_area = gkrellm_scrolled_text(vbox, NULL,
                                      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	for(i = 0; i < sizeof(help_text) / sizeof(gchar *); ++i) {
		gkrellm_add_info_text_string(help_area, help_text[i]);
	}
#else
    vbox      = gkrellm_gtk_framed_notebook_page(notebook, "Help");
    help_area = gkrellm_gtk_scrolled_text_view(vbox, NULL, 
                                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	for(i = 0; i < sizeof(help_text) / sizeof(gchar *); ++i) {
		gkrellm_gtk_text_view_append(help_area, help_text[i]);
	}
#endif
}


static void create_about_tab(GtkWidget *notebook)
{
    gchar *about;
    GtkWidget *about_label;
    GtkWidget *tab_label;
        
    about = g_strdup_printf(about_text, GKRELLMBUPS_VERSION_MAJOR, GKRELLMBUPS_VERSION_MINOR, GKRELLMBUPS_VERSION_REV);
    about_label = gtk_label_new(about);
    gtk_widget_show(about_label);
    g_free(about);

    tab_label = gtk_label_new("About");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), about_label, tab_label);
}


void bups_create_gui(GtkWidget *tab)
{
    GtkWidget *note;

    note = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(note), GTK_POS_TOP);
    gtk_box_pack_start(GTK_BOX(tab), note, TRUE, TRUE, 0);

    create_options_tab(note);
    create_toggles_tab(note);
    create_help_tab(note);
    create_about_tab(note);
}


/*****************************************************************************\
* Config structure and file functions.                                        *
\*****************************************************************************/ 

/** Create and initialise the config structure to default values.
 */
BUPSConfig *bups_create_config(void)
{
    /* hope and pray this works... :/ */
    BUPSConfig *config = g_new0(BUPSConfig, 1); 

    /* set default configuration */
    config -> mode         = DEFAULT_MODE;
    config -> pro_net      = g_strdup(DEFAULT_PRONET);
    config -> belkin_host  = g_strdup(DEFAULT_BELKIN_HOST);
    config -> belkin_port  = DEFAULT_BELKIN_PORT;
    config -> nut_host     = g_strdup(DEFAULT_NUT_HOST);
    config -> nut_port     = DEFAULT_NUT_PORT;
    config -> nut_username = NULL;
    config -> nut_password = NULL;
    config -> show_log     = FALSE;
    config -> mains        = MAINS_MIN;
    config -> show_volt    = TRUE;
    config -> show_freq    = TRUE;
    config -> show_stat    = TRUE;
    config -> show_msgs    = TRUE;

    if(file_selector == NULL) {
        file_selector = create_fileselect();
    }

    return config;
}


/** Save the user settings.
 *  Write the configuration data to the specified file. Note that some of the 
 *  values come from the config structure, but the show chart texts, which are
 *  only proxied in the config structure, are taken from the chart data structs. 
 */
void bups_save_config(FILE *file)
{
    /*  config structure */
    fprintf(file, "%s mode %d\n"        , MONITOR_CONFIG_KEYWORD, bups_data -> config -> mode);
    fprintf(file, "%s pronet %s\n"      , MONITOR_CONFIG_KEYWORD, bups_data -> config -> pro_net);
    fprintf(file, "%s belkin_host %s\n" , MONITOR_CONFIG_KEYWORD, bups_data -> config -> belkin_host);
    fprintf(file, "%s belkin_port %d\n" , MONITOR_CONFIG_KEYWORD, bups_data -> config -> belkin_port);
    fprintf(file, "%s nut_host %s\n"    , MONITOR_CONFIG_KEYWORD, bups_data -> config -> nut_host);
    fprintf(file, "%s nut_port %d\n"    , MONITOR_CONFIG_KEYWORD, bups_data -> config -> nut_port);
    fprintf(file, "%s nut_auth %d\n"    , MONITOR_CONFIG_KEYWORD, bups_data -> config -> nut_auth);
    fprintf(file, "%s nut_username %s\n", MONITOR_CONFIG_KEYWORD, bups_data -> config -> nut_username);
    fprintf(file, "%s nut_password %s\n", MONITOR_CONFIG_KEYWORD, bups_data -> config -> nut_password);
    fprintf(file, "%s showlog %d\n"     , MONITOR_CONFIG_KEYWORD, bups_data -> config -> show_log);
    fprintf(file, "%s mains %d\n"       , MONITOR_CONFIG_KEYWORD, bups_data -> config -> mains);

    fprintf(file, "%s showvolt %d\n"    , MONITOR_CONFIG_KEYWORD, bups_data -> config -> show_volt);
    fprintf(file, "%s showfreq %d\n"    , MONITOR_CONFIG_KEYWORD, bups_data -> config -> show_freq);
    fprintf(file, "%s showstat %d\n"    , MONITOR_CONFIG_KEYWORD, bups_data -> config -> show_stat);
    fprintf(file, "%s showmsgs %d\n"    , MONITOR_CONFIG_KEYWORD, bups_data -> config -> show_msgs);


    /* chart structures */
    fprintf(file, "%s volt_format %s\n", MONITOR_CONFIG_KEYWORD, bups_data -> volt_chart.text_format);
    fprintf(file, "%s freq_format %s\n", MONITOR_CONFIG_KEYWORD, bups_data -> freq_chart.text_format);
    fprintf(file, "%s temp_format %s\n", MONITOR_CONFIG_KEYWORD, bups_data -> temp_chart.text_format);
    fprintf(file, "%s show_volt %d\n"  , MONITOR_CONFIG_KEYWORD, bups_data -> volt_chart.show_text);
    fprintf(file, "%s show_freq %d\n"  , MONITOR_CONFIG_KEYWORD, bups_data -> freq_chart.show_text);
    fprintf(file, "%s show_temp %d\n"  , MONITOR_CONFIG_KEYWORD, bups_data -> temp_chart.show_text);

	gkrellm_save_chartconfig(file, bups_data -> volt_chart.config, MONITOR_CONFIG_KEYWORD, "volt");
	gkrellm_save_chartconfig(file, bups_data -> freq_chart.config, MONITOR_CONFIG_KEYWORD, "freq");
	gkrellm_save_chartconfig(file, bups_data -> temp_chart.config, MONITOR_CONFIG_KEYWORD, "temp");
}


/** Load the user settings.
 *  Attepts to extract Useful Information(tm) from a string presented to the
 *  function buy GKrellM. Things are slightly more complicated than the average
 *  case by the fact that we need to distinguish between three chartconfig lines
 *  rather than the single chart must plugins deal with.
 */
void bups_load_config(gchar *line)
{
    gchar keyword[31], name[31];
    gchar data[CONFIG_BUFSIZE], conf[CONFIG_BUFSIZE];

    if(2 == sscanf(line, "%31s %[^\n]", keyword, data)) {
        /* config structure */
        if(!strcmp(keyword, "mode")) {
            bups_data -> config -> mode = strtol(data, NULL, 10);        
        } else if(!strcmp(keyword, "pronet")) {
            gkrellm_dup_string(&bups_data -> config -> pro_net, data);
        } else if(!strcmp(keyword, "belkin_host")) {
            gkrellm_dup_string(&bups_data -> config -> belkin_host, data);
        } else if(!strcmp(keyword, "belkin_port")) {
           bups_data ->  config -> belkin_port = strtol(data, NULL, 10);
        } else if(!strcmp(keyword, "nut_host")) {
            gkrellm_dup_string(&bups_data -> config -> nut_host, data);
        } else if(!strcmp(keyword, "nut_port")) {
           bups_data ->  config -> nut_port = strtol(data, NULL, 10);
        } else if(!strcmp(keyword, "nut_auth")) {
           bups_data ->  config -> nut_auth = strtol(data, NULL, 10);
        } else if(!strcmp(keyword, "nut_username")) {
            gkrellm_dup_string(&bups_data -> config -> nut_username, data);
        } else if(!strcmp(keyword, "nut_password")) {
            gkrellm_dup_string(&bups_data -> config -> nut_password, data);
        } else if(!strcmp(keyword, "mains")) {
            bups_data -> config -> mains = strtol(data, NULL, 10);
        } else if(!strcmp(keyword, "showlog")) {
            bups_data -> config -> show_log = strtol(data, NULL, 10);
        } else if(!strcmp(keyword, "showvolt")) {
            bups_data -> config -> show_volt = strtol(data, NULL, 10);
        } else if(!strcmp(keyword, "showfreq")) {
            bups_data -> config -> show_freq = strtol(data, NULL, 10);
        } else if(!strcmp(keyword, "showstat")) {
            bups_data -> config -> show_stat = strtol(data, NULL, 10);
        } else if(!strcmp(keyword, "showmsgs")) {
            bups_data -> config -> show_msgs = strtol(data, NULL, 10);

        /* Chart structures */ 
        } else if(!strcmp(keyword, "volt_format")) {
            gkrellm_dup_string(&bups_data -> volt_chart.text_format, data);
        } else if(!strcmp(keyword, "freq_format")) {
            gkrellm_dup_string(&bups_data -> freq_chart.text_format, data);
        } else if(!strcmp(keyword, "temp_format")) {
            gkrellm_dup_string(&bups_data -> temp_chart.text_format, data);
        } else if(!strcmp(keyword, "show_volt")) {
            bups_data -> volt_chart.show_text = strtol(data, NULL, 10);
        } else if(!strcmp(keyword, "show_freq")) {
            bups_data -> freq_chart.show_text = strtol(data, NULL, 10);
        } else if(!strcmp(keyword, "show_temp")) {
            bups_data -> temp_chart.show_text = strtol(data, NULL, 10);
        } else if(!strcmp(keyword, GKRELLM_CHARTCONFIG_KEYWORD)) {

            /* line is a chartconfig, need to do a bit more parsing to work out 
             * which chart this is the config for..
             */
            if(2 == sscanf(data, "%31s %[^\n]", name, conf)) {
                if(!strcmp(name, "volt")) {
                    gkrellm_load_chartconfig(&bups_data -> volt_chart.config, conf, 3);
                } else if(!strcmp(name, "freq")) {
                    gkrellm_load_chartconfig(&bups_data -> freq_chart.config, conf, 2);
                } else if(!strcmp(name, "temp")) {
                    gkrellm_load_chartconfig(&bups_data -> temp_chart.config, conf, 2);
                }
            }
        }
    }
}


/** Update the plugin configuration based on values in the user interface.
 *  This copies the values from the gadgets in the tab created by createTab()
 *  into the config structure. Note that this will only restart the client
 *  when the host or port have changed!
 */
void bups_apply_config(void)
{
    const gchar    *contents;
    gint      portset, oldmode;
    gboolean  update_local  = FALSE;
    gboolean  update_remote = FALSE;

#ifdef ENABLE_NUT
    gboolean update_nut    = FALSE;
#endif

    bups_data -> config -> show_volt = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_volt));
    bups_data -> config -> show_freq = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_freq));
    bups_data -> config -> show_stat = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_stats));
    bups_data -> config -> show_msgs = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_msgs));

    /* show/hide sections */
    if(bups_data -> config -> show_volt) {
        gkrellm_chart_show(bups_data -> volt_chart.chart, TRUE);
    } else {
        gkrellm_chart_hide(bups_data -> volt_chart.chart, TRUE);
    }

    if(bups_data -> config -> show_freq) {
        gkrellm_chart_show(bups_data -> freq_chart.chart, TRUE);
    } else {
        gkrellm_chart_hide(bups_data -> freq_chart.chart, TRUE);
     }

    if(bups_data -> config -> show_stat) {
        gkrellm_chart_show(bups_data -> temp_chart.chart, TRUE);
    } else {
        gkrellm_chart_hide(bups_data -> temp_chart.chart, TRUE);
    }

    if(bups_data -> config -> show_msgs) {
        gkrellm_panel_show(bups_data -> log_display);
    } else {
        gkrellm_panel_hide(bups_data -> log_display);
    }

    contents = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(voltage_combo)->entry));
    gkrellm_dup_string(&bups_data -> volt_chart.text_format, (gchar *)contents);

    contents = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(frequency_combo)->entry));
    gkrellm_dup_string(&bups_data -> freq_chart.text_format, (gchar *)contents);

    contents = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(temperature_combo)->entry));
    gkrellm_dup_string(&bups_data -> temp_chart.text_format, (gchar *)contents);

    contents = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(mains_combo)->entry));    
    bups_data -> config -> mains = strtol(contents, NULL, 0);

    /* local belkin */
    contents = gtk_entry_get_text(GTK_ENTRY(pronet_location));
    update_local = gkrellm_dup_string(&bups_data -> config -> pro_net, contents);

    /* remote belkin */
    contents = gtk_entry_get_text(GTK_ENTRY(remote_host));
    update_remote = gkrellm_dup_string(&bups_data -> config -> belkin_host, contents);
    portset  = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(remote_port));
    if(bups_data -> config -> belkin_port != portset) {
        bups_data -> config -> belkin_port = portset;
        update_remote = TRUE;
    }

#ifdef ENABLE_NUT
    /* nut */
    contents = gtk_entry_get_text(GTK_ENTRY(nut_host));
    update_nut = gkrellm_dup_string(&bups_data -> config -> nut_host, contents);
    portset  = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(nut_port));
    if(bups_data -> config -> nut_port != portset) {
        bups_data -> config -> nut_port = portset;
        update_nut = TRUE;
    }
#endif
 
    oldmode = bups_data -> config -> mode;
    bups_data -> config -> mode = activemode;
    if(activemode == oldmode) {
        oldmode = 1;
    } else { 
        oldmode = 0;
    }

    /* Only restart the client if we really have to as this can have a visible
     * effect on updates of the main window.
     */
    /* mode change */         
    if(!oldmode || 
#ifdef ENABLE_NUT
       (update_nut    && (activemode == 2)) ||  /* changed nut and mode is nut       */
#endif
       (update_local  && (activemode == 0)) ||  /* changed local and mode is local   */
       (update_remote && (activemode == 1))) {  /* changed remote and mode is remote */
        halt_client(bups_data -> client);
        bups_data -> client = launch_client(bups_data -> config);
    }
}
