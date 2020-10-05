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
 *  \file chart.c
 *  This file contains all the functions used to create, update and draw the 
 *  plugin and some callbacks used to open various configuration windows.
 * 
 */
/*  $Id: chart.c,v 1.2 2003/02/06 21:07:53 chris Exp $
 */

#include"chart.h"
#include"gkrellmbups.h"
#include"ups_connect.h"

/*! Convenience macro to make limiting values to l or greater easier. */
#define LIM_FLOOR(x, l) ((x) < (l)) ? (l) : (x)

#define	MIN_GRID_RES         1              /*!< Constrain grid resolution, lower end. */ 
#define	MAX_GRID_RES         40             /*!< Constrain grid resolution, upper end. */              

#define DEFAULT_CHARTHEIGHT  40             /*!< 40 is probably a good trade between detail and screen use */  

/*! chart config names for the voltage chart data entries     */
static gchar *volt_names[] = { "Input voltage", "Output voltage", "Battery voltage", NULL };

/*! chart config names for the frequency chart data entries   */
static gchar *freq_names[] = { "Input frequency", "Output frequency", NULL };

/*! chart config names for the temperature chart data entries */
static gchar *temp_names[] = { "Temperature", "Load", NULL };


/*****************************************************************************\
* Chart text formatting functions.                                            *
\*****************************************************************************/ 

/** Voltage chart text formatter.
 *  This replaces special "$" codes in the specified format sttring with
 *  values taken from upsStatus. Please see the switch in the body of the
 *  function for details about which codes are recognised. All unrecognised
 *  codes or other characters are simply copied to the buffer.
 *
 *  \par Arguments:
 *  \arg \c buffer - Destination buffer.
 *  \arg \c size - number of characters available in buffer (not including newline).
 *  \arg \c format - Format string to process into buffer.
 *
 *  \todo Find a way to unify formatVoltText(), formatFreqText() and 
 *  formatTempText() to reduce the amount of replication between them (maybe
 *  use varargs and pass the values to replace code with in that way?)
 */
/*  NOTE: Safe for 1.0 and 2.0 
 */
static void format_volt_text(gchar *buffer, gint size, gchar *format)
{
    gchar *fpos;
    gchar  opt;
    gint   len;

    size--;
    *buffer = '\0';

    if(format) {
        for(fpos = format; (*fpos != '\0') && (size != 0); fpos ++) {
            len = 1;
            if((*fpos == '$') && (*(fpos + 1) != '\0')) {
                opt = *(fpos + 1);
                switch(opt) {
                    /* $i - input voltage (in volts) */
                    case 'i': len = snprintf(buffer, size, "%3.1f", ups_status.in_Voltage ); fpos ++; break;
                    /* $o - output voltage (in volts) */
                     case 'o': len = snprintf(buffer, size, "%3.1f", ups_status.out_Voltage); fpos ++; break;
                    /* $b - batteryvoltage (in volts) */
                    case 'b': len = snprintf(buffer, size, "%3.1f", ups_status.bat_Voltage); fpos ++; break;
                    /* $l - battery level as a percentage. */
                    case 'l': len = snprintf(buffer, size, "%3.1f", ups_status.bat_Level  ); fpos ++; break;
                    default: *buffer = *fpos; break;
                }
            } else {
                *buffer = *fpos;
            }
            
            size -= len;
            buffer += len;
        }
        *buffer = '\0';
    } else {
        g_snprintf(buffer, size, "No format");
    }  
}


/** Frequency chart text formatter.
 *  This does much the same thing as formatVoltText() except for the frequency chart.
 *
 *  \par Arguments:
 *  \arg \c buffer - Destination buffer.
 *  \arg \c size - number of characters available in buffer (not including newline).
 *  \arg \c format - Format string to process into buffer.
 */
/*  NOTE: Safe for 1.0 and 2.0 
 */
static void format_freq_text(gchar *buffer, gint size, gchar *format)
{
    gchar *fpos;
    gchar  opt;
    gint   len;

    size--;
    *buffer = '\0';

    if(format) {
        for(fpos = format; (*fpos != '\0') && (size != 0); fpos ++) {
            len = 1;
            if((*fpos == '$') && (*(fpos + 1) != '\0')) {
                opt = *(fpos + 1);
                switch(opt) {
                    case 'i': len = snprintf(buffer, size, "%2.1f", ups_status.in_Freq ); fpos ++; break;
                    case 'o': len = snprintf(buffer, size, "%2.1f", ups_status.out_Freq); fpos ++; break;
                    default: *buffer = *fpos; break;
                }
            } else {
                *buffer = *fpos;
            }

            size -= len;
            buffer += len;
        }
        *buffer = '\0';
    } else {
        g_snprintf(buffer, size, "No format");
    }  
}


/** Frequency chart text formatter.
 *  This does much the same thing as formatVoltText() except for the 
 *  temperature chart.
 *
 *  \par Arguments:
 *  \arg \c buffer - Destination buffer.
 *  \arg \c size - number of characters available in buffer (not including newline).
 *  \arg \c format - Format string to process into buffer.
 */
/*  NOTE: Safe for 1.0 and 2.0 
 */
static void format_temp_text(gchar *buffer, gint size, gchar *format)
{
    gchar *fpos;
    gchar  opt;
    gint   len;

    size--;
    *buffer = '\0';

    if(format) {
        for(fpos = format; (*fpos != '\0') && (size != 0); fpos ++) {
            len = 1;
            if((*fpos == '$') && (*(fpos + 1) != '\0')) {
                opt = *(fpos + 1);
                switch(opt) {
                    case 't': len = snprintf(buffer, size, "%2.1f", ups_status.ups_Temp); fpos ++; break;
                    case 'l': len = snprintf(buffer, size, "%3.1f", ups_status.ups_Load); fpos ++; break;
                    default: *buffer = *fpos; break;
                }
            } else {
                *buffer = *fpos;
            }

            size -= len;
            buffer += len;
        }
        *buffer = '\0';
    } else {
        g_snprintf(buffer, size, "No format");
    }  
}


/*****************************************************************************\
* Chart and panel drawing functions.                                          *
\*****************************************************************************/ 

/** Draw the chart data and, optionally, text overlay. 
 *  As the user can opt to have a text over on the charts, this function
 *  is required to handle the drawing. 
 */  
/*  WARN: Safe for 1.0 and 2.0, with correct config structure changes. 
 */
static void draw_chart(BUPSChart *chart)
{
    chart -> draw_buffer[0] = '\0';

	gkrellm_draw_chartdata(chart -> chart);
    if(chart -> show_text) {
        chart -> format(chart -> draw_buffer, DRAW_BUFFER_SIZE, chart -> text_format);
        gkrellm_draw_chart_text(chart -> chart, bups_style_id, chart -> draw_buffer);
    }
	gkrellm_draw_chart_to_screen(chart -> chart);
}


/** Draw the log panel, either drawing a static label or a scrolling log.
 *  This function handles the drawing of the ups "log message" panel, either
 *  showing a static "UPS" label or scrolling the last log message from
 *  the UPS service.
 */
/*  WARN: Safe for 1.0 and 2.0, with correct config structure changes. 
 */
static void draw_log(void)
{
    gint width;

    if(bups_data -> config -> show_log) {
        /* This next bit is taken from gkrellweather - I much prefer this scrolling 
         * setup to the more jumpy version used in some of the other panels
         */
        width = gkrellm_chart_width();
        bups_data -> log_scr = (bups_data -> log_scr + 1) % (2 * width);
        bups_data -> log_decal -> x_off = width - bups_data -> log_scr;
        if(bups_data -> log_text) {
            gkrellm_draw_decal_text(bups_data -> log_display, bups_data -> log_decal, bups_data -> log_text, width - bups_data -> log_scr);
        } else {
            /* FIXME!! locking?!? */
            if(ups_status.ups_Present) {
                gkrellm_draw_decal_text(bups_data -> log_display, bups_data -> log_decal, "No log messsage waiting.", width - bups_data -> log_scr);
            } else {
                gkrellm_draw_decal_text(bups_data -> log_display, bups_data -> log_decal, "No UPS detected!", width - bups_data -> log_scr);
            }
        }
    } else {
        bups_data -> label_decal -> x_off = bups_data -> label_x;
        gkrellm_draw_decal_text(bups_data -> log_display, bups_data -> label_decal, bups_data -> log_label, -1);
    }
}


/** Callback for handling chart ExposeEvent events.
 *  This will draw the backing pixmap for the relevant part of a panel
 *  or chart - note that unlike the examples (and indeed almost every
 *  other plugin I've looked at) this uses the event userdata pointer
 *  to optimise the pixmap identification process.
 */
/*  NOTE: 2.0 safe only. gdk_draw_drawable not in GDK 1.x
 */
static gint chart_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    BUPSChart *chart  = (BUPSChart *)data;
    GdkPixmap *pixmap = NULL;

    /* find out exactly what has been exposed...*/
    if(widget == chart -> chart -> drawing_area) {
        pixmap = chart -> chart -> pixmap;
    } else if(widget == chart -> panel -> drawing_area) {
        pixmap = chart -> panel -> pixmap;
    }

    /* If we are responsible redraw it! */
    if(pixmap) {
        gdk_draw_drawable(widget -> window,
                          widget -> style -> fg_gc[GTK_WIDGET_STATE(widget)],
                          pixmap, 
                          event -> area.x, event -> area.y, 
                          event -> area.x, event -> area.y,
                          event -> area.width, event -> area.height);
    }
	return FALSE;
}


/** Callback for handling ExposeEvent events sent to the log panel.
 *  Couldn't get much easier than this - just a simple pixmap copy! :)
 */
/*  NOTE: 2.0 safe only. gdk_draw_drawable not in GDK 1.x
 */
static gint expose_log_event(GtkWidget *widget, GdkEventExpose *event)
{
     gdk_draw_drawable(widget -> window,
                       widget -> style -> fg_gc[GTK_WIDGET_STATE(widget)],
                       bups_data -> log_display -> pixmap, 
                       event -> area.x, event -> area.y, 
                       event -> area.x, event -> area.y,
                       event -> area.width, event -> area.height);
	return FALSE;
}


/** Callback for handling button events sent to the charts.
 *  Pressing the right mouse button, or double-left-clicking will open the 
 *  chartcofig window for the chart the user has selected. Single-left 
 *  clicking toggles the chart text overlay function.
 */
/*  NOTE: 2.0 safe only. 
 */
static void cb_chart_click(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    BUPSChart *target = (BUPSChart *)data;

	if((event -> button == 3) || (event -> button == 1 && event -> type == GDK_2BUTTON_PRESS)) {
		gkrellm_chartconfig_window_create(target -> chart);
	} else if((event -> button == 1) && (event -> type == GDK_BUTTON_PRESS)) {
        target -> show_text = !target -> show_text;
        gkrellm_config_modified();
        draw_chart(target);
    }
}


/** Callback for handling button events sent to the log panel.
 *  A single right click will open the plugin configuration while lief-clicking
 *  toggles between the label and scrolling log displays.
 */
/*  WARN: Safe for 1.0 and 2.0, with correct config structure changes. 
 */
static void cb_log_click(GtkWidget *widget, GdkEventButton *event)
{
	if(event -> button == 3) {
		gkrellm_open_config_window(bups_mon);
	} else if(event -> button == 2) {
        if(bups_data -> config -> show_log) {
            gkrellm_make_decal_invisible(bups_data -> log_display, bups_data -> log_decal);
            bups_data -> config -> show_log = FALSE;
            draw_log();
            gkrellm_make_decal_visible(bups_data -> log_display, bups_data -> label_decal);
        } else {
            gkrellm_make_decal_invisible(bups_data -> log_display, bups_data -> label_decal);
            bups_data -> config -> show_log = TRUE;
            draw_log();
            gkrellm_make_decal_visible(bups_data -> log_display, bups_data -> log_decal);
        }
		gkrellm_config_modified();
    }    
}


/*****************************************************************************\
* Creation and update functions.                                              *
\*****************************************************************************/ 

/** Add latest chart values and check for log updates.
 *  Called fairly regularly, but this only does anythignn really interesting once
 *  a second - it locks the mutex on ups_status and updates all three charts to
 *  the latest values from the client thread. Once done the log string is 
 *  checked and possibly duplicated. 
 */ 
/*  NOTE: 2.0 safe only, uses glib 2 mutex
 */
void bups_update_plugin(void)
{
    gint vala, valb, valc;

    if(GK.second_tick) {
        if(ups_status_lock) g_mutex_lock(ups_status_lock); /* best to do this even though we aren't writing */
        vala = LIM_FLOOR((gint)ups_status.in_Voltage - bups_data -> config -> mains, 0);
        valb = LIM_FLOOR((gint)ups_status.out_Voltage - bups_data -> config -> mains, 0);
        valc = LIM_FLOOR((gint)ups_status.bat_Voltage, 0);
        gkrellm_store_chartdata(bups_data -> volt_chart.chart, 0, vala, valb, valc);
        draw_chart(&bups_data -> volt_chart);

        vala = LIM_FLOOR((gint)ups_status.in_Freq, 0);
        valb = LIM_FLOOR((gint)ups_status.out_Freq, 0);
        gkrellm_store_chartdata(bups_data -> freq_chart.chart, 0, vala, valb);
        draw_chart(&bups_data -> freq_chart);

        vala = LIM_FLOOR((gint)ups_status.ups_Temp, 0);
        valb = LIM_FLOOR((gint)ups_status.ups_Load, 0);
        gkrellm_store_chartdata(bups_data -> temp_chart.chart, 0, vala, valb);
        draw_chart(&bups_data -> temp_chart);

        /* this bit MUST be inside a mutex on ups_status or heaven knows what will happen when the 
         * thread updates ups_LastLog half way through the strdup ... 
         */
        vala = strlen(ups_status.ups_LastLog);
        if(vala) {
            gkrellm_dup_string(&bups_data -> log_text, ups_status.ups_LastLog);
        }
        if(ups_status_lock) g_mutex_unlock(ups_status_lock);
    }
    draw_log();
    gkrellm_draw_panel_layers(bups_data -> log_display);
}


/** Create a new BUPSData chart.
 *  Simple enough to describe - this creates charts. What it actually does is more
 *  complicated, but that is best highlighted via th arguments:
 *
 *  \par Arguments:
 *  \arg \c vbox - the box into which a vbox containing a chart and panel should be added.
 *  \arg \c data - the BUPSData structure to fill in.
 *  \arg \c firstCreate - TRUE when this is the first tiem this has been called.
 *  \arg \c dataNames - array of names, one for each chartdata element (last element should be NULL).
 *  \arg \c name - Text to display as a label in the panel below the chart.
 *  \arg \c format - pointer to the function which formats this charts text overlay.
 */
/*  NOTE: 2.0 safe only, uses GTK 2 signal model 
 */
static void create_chart(GtkWidget *vbox, BUPSChart *data, gint firstCreate, gchar *dataNames[], gchar *name, void (*format)(gchar *, gint, gchar*))
{
    int count = 0;
 
    if(firstCreate) {
        /* Create a vbox into whcih the chart and panel can be added */
        data -> vbox = gtk_vbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(vbox), data -> vbox);
        gtk_widget_show(data -> vbox);

        /* Chart and panel creation... */
		data -> chart = gkrellm_chart_new0();
        data -> panel = data -> chart -> panel = gkrellm_panel_new0();
        data -> format = format;
    }

    gkrellm_set_chart_height_default(data -> chart, DEFAULT_CHARTHEIGHT);
    gkrellm_chart_create(data -> vbox, bups_mon, data -> chart, &data -> config);

    while((count < MAX_DATA) && dataNames[count]) {
        data -> data[count] = gkrellm_add_default_chartdata(data -> chart, dataNames[count]);
        gkrellm_monotonic_chartdata(data -> data[count], FALSE);
        gkrellm_set_chartdata_draw_style_default(data -> data[count], CHARTDATA_LINE);
        gkrellm_set_chartdata_flags(data -> data[count], CHARTDATA_ALLOW_HIDE);
        count ++;
    }

	/* Set your own chart draw function if you have extra info to draw */
	gkrellm_set_draw_chart_function(data -> chart, draw_chart, data);

	/* If this next call is made, then there will be a resolution spin
	 *  button on the chartconfig window so the user can change resolutions.
	 */
	gkrellm_chartconfig_grid_resolution_adjustment(data -> config, 
                                                   TRUE,
                                                   0, 
                                                   (gfloat) MIN_GRID_RES, 
                                                   (gfloat) MAX_GRID_RES, 
                                                   0, 0, 0, 70);
	gkrellm_chartconfig_grid_resolution_label(data -> config, "Units drawn on the chart");

    gkrellm_panel_configure(data -> panel, name, gkrellm_panel_style(bups_style_id));
    gkrellm_panel_create(data -> vbox, bups_mon, data -> panel);

	gkrellm_alloc_chartdata(data -> chart);

    if(firstCreate) {
        /* callbacks to redraw the widgets. As far as I can tell, most gkrellm plugins
         * (and certainly the built-in meters) ignore the user data for these: here we
         * can drmatically simplify the buton click and expose event handlers by passing
         * the BUPSData structure as the userdata.
         */
        g_signal_connect(G_OBJECT(data -> chart -> drawing_area),
                         "expose_event", 
                         G_CALLBACK(chart_expose_event), (gpointer)data);
        g_signal_connect(G_OBJECT(data -> panel -> drawing_area),
                         "expose_event", 
                         G_CALLBACK(chart_expose_event), (gpointer)data);
        /* callback to handle mouse clicks on the chart */
		g_signal_connect(G_OBJECT(data -> chart -> drawing_area),
                         "button_press_event", 
                         G_CALLBACK(cb_chart_click), (gpointer)data);
    } else {
		draw_chart(data);
	}
}


/** Create the plugin charts and panels. 
 *  Much of the actual work for this is done by the createChart() function, only 
 *  the log display panel is actually created in teh body of this function - 
 *  createchart is called three times; once for the voltage display, once for 
 *  the frequency and once for the temperature. With a bit of fiddling it may
 *  be possible to make charts optional, but that's one for a future version..
 */
/*  NODE: 2.0 safe only, uses GTK 2 signal model. 
 */
void bups_create_plugin(GtkWidget *vbox, gint firstCreate)
{
    gint labelWidth;
    GdkFont* font;

    if(firstCreate) {
        bups_data -> vbox = gtk_vbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(vbox), bups_data -> vbox);
        gtk_widget_show(bups_data -> vbox);

        bups_data -> log_display = gkrellm_panel_new0();
        bups_data -> log_label   = "UPS";
        bups_data -> client     = launch_client(bups_data -> config);
    }
    
    create_chart(bups_data -> vbox, &bups_data -> volt_chart, firstCreate, volt_names, "Voltages", format_volt_text);
    create_chart(bups_data -> vbox, &bups_data -> freq_chart, firstCreate, freq_names, "Freq"    , format_freq_text);
    create_chart(bups_data -> vbox, &bups_data -> temp_chart, firstCreate, temp_names, "Stats"   , format_temp_text);

	bups_data -> log_style = gkrellm_meter_style(bups_style_id);
    bups_data -> log_decal = gkrellm_create_decal_text(bups_data -> log_display, "Afp0",
                                                     gkrellm_meter_alt_textstyle(bups_style_id), 
                                                     bups_data -> log_style, -1, -1, -1);
    
    bups_data -> label_decal = gkrellm_create_decal_text(bups_data -> log_display, bups_data -> log_label,
                                                     gkrellm_meter_textstyle(bups_style_id), 
                                                     bups_data -> log_style, -1, -1, -1);

    font = gdk_font_from_description(bups_data -> label_decal -> text_style.font);
    labelWidth = gdk_string_width(font, bups_data -> log_label);
    if(labelWidth < bups_data -> label_decal -> w) {
        bups_data -> label_x = (bups_data -> label_decal -> w - labelWidth) / 2;
    } else {
        bups_data -> label_x = 0;
    }

    bups_data -> log_scr = 0;

	gkrellm_panel_configure(bups_data -> log_display, NULL, bups_data -> log_style);
	gkrellm_panel_create(vbox, bups_mon, bups_data -> log_display);

     
    bups_data -> config -> show_log = !bups_data -> config -> show_log;
    draw_log();
    bups_data -> config -> show_log = !bups_data -> config -> show_log;
    draw_log();

    if(bups_data -> config -> show_log) {
        gkrellm_make_decal_visible(bups_data -> log_display, bups_data -> log_decal);
        gkrellm_make_decal_invisible(bups_data -> log_display, bups_data -> label_decal);
    } else {
        gkrellm_make_decal_invisible(bups_data -> log_display, bups_data -> log_decal);
        gkrellm_make_decal_visible(bups_data -> log_display, bups_data -> label_decal);
    }

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

    if(firstCreate) {
 		g_signal_connect(G_OBJECT(bups_data -> log_display -> drawing_area), 
                         "expose_event",
                         G_CALLBACK(expose_log_event), NULL);
		g_signal_connect(G_OBJECT(bups_data -> log_display -> drawing_area),
                         "button_press_event", 
                         G_CALLBACK(cb_log_click), NULL);
    }
}
