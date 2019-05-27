/*
 *  Copyright (C) 2003 Jan C. Depner, Jack O'Quin, Steve Harris
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  $Id: callbacks.c,v 1.181 2013/02/09 15:47:29 kotau Exp $
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>


#include "main.h"
#include "support.h"
#include "callbacks.h"
#include "callbacks_help.h"
#include "geq.h"
#include "hdeq.h"
#include "interface.h"
#include "process.h"
#include "intrim.h"
#include "compressor-ui.h"
#include "gtkmeter.h"
//#include "gtkmeterscale.h"
#include "state.h"
#include "db.h"
#include "presets-ui.h"
#include "multiout-ui.h"
#include "status-ui.h"
#include "limiter-ui.h"
#include "io-menu.h"
#include "transport.h"
#include "scenes.h"
#include "spectrum.h"
#include "preferences.h"
#include "help.h"


/* vi:set ts=8 sts=4 sw=4: */


static char             *help_ptr = NULL, scene_name_text[100];
static gboolean         text_focus = FALSE, force_keypress_help = FALSE;
static GtkToggleButton  *l_solo_button[XO_NBANDS], *l_bypass_button[XO_NBANDS], 
                        *l_global_bypass, *l_eq_bypass, *l_limiter_bypass,
                        *l_low_delay,*l_mid_delay;
static GtkLabel         *l_eqbFreqLabel, *l_eqbAmpLabel;
GtkEventBox             *l_global_bypass_event_box, *l_comp_bypass_event_box[XO_NBANDS],
                        *l_eq_bypass_event_box, *l_limiter_bypass_event_box;
static int              hot_scene = 0;
static GtkWidget        *scene_name_dialog, *about_dialog;
static GtkEntry         *l_scene_name_entry;
static GdkColor         l_main_color, l_eq_color, l_comp_color[XO_NBANDS], l_limiter_color;


void
on_low2mid_value_changed               (GtkRange        *range,
                                        gpointer         user_data)
{
    hdeq_low2mid_set (range);
}


void
on_mid2high_value_changed              (GtkRange        *range,
                                        gpointer         user_data)
{
    hdeq_mid2high_set (range);
}


gboolean
on_low2mid_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    hdeq_low2mid_button (1);

    set_scene_warning_button ();

    return FALSE;
}


gboolean
on_low2mid_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    hdeq_low2mid_button (0);

    return FALSE;
}


gboolean
on_mid2high_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    hdeq_mid2high_button (1);

    set_scene_warning_button ();

    return FALSE;
}


gboolean
on_mid2high_button_release_event      (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    hdeq_mid2high_button (0);

    return FALSE;
}


void
on_low2mid_realize                     (GtkWidget       *widget,
                                        gpointer         user_data)
{
    hdeq_low2mid_init ();
}


void
on_mid2high_realize                    (GtkWidget       *widget,
                                        gpointer         user_data)
{
    hdeq_mid2high_init ();
}


void
on_quit_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
    clean_quit ();
}


gboolean
on_window1_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    clean_quit ();

    return FALSE;
}


void
on_window1_show                        (GtkWidget       *widget,
                                        gpointer         user_data)
{
  GtkStyle *main_style, *eq_style, *comp_style[XO_NBANDS], *limiter_style;

  help_ptr = _(general_help);

  crossover_init ();

  l_notebook1 = GTK_NOTEBOOK (lookup_widget (main_window, "notebook1"));

  on_EQ_curve_event_box_leave_notify_event (NULL, NULL, NULL);

  l_solo_button[0] = GTK_TOGGLE_BUTTON (lookup_widget (main_window, "low_solo"));
  l_solo_button[1] = GTK_TOGGLE_BUTTON (lookup_widget (main_window, "mid_solo"));
  l_solo_button[2] = GTK_TOGGLE_BUTTON (lookup_widget (main_window, "high_solo"));

  l_eqbFreqLabel = GTK_LABEL (lookup_widget (main_window, "eqbFreqLabel"));
  l_eqbAmpLabel = GTK_LABEL (lookup_widget (main_window, "eqbAmpLabel"));

  l_eq_bypass_event_box = GTK_EVENT_BOX (lookup_widget (main_window, "eq_bypass_event_box"));
  l_eq_bypass = GTK_TOGGLE_BUTTON (lookup_widget (main_window, "eq_bypass"));
  eq_style = gtk_widget_get_style (GTK_WIDGET (l_eq_bypass_event_box));
  l_eq_color = eq_style->bg[GTK_STATE_NORMAL];

  l_comp_bypass_event_box[0] = GTK_EVENT_BOX (lookup_widget (main_window, "low_bypass_event_box"));
  l_bypass_button[0] = GTK_TOGGLE_BUTTON (lookup_widget (main_window, "low_bypass"));
  comp_style[0] = gtk_widget_get_style (GTK_WIDGET (l_comp_bypass_event_box[0]));
  l_comp_color[0] = comp_style[0]->bg[GTK_STATE_NORMAL];

  l_comp_bypass_event_box[1] = GTK_EVENT_BOX (lookup_widget (main_window, "mid_bypass_event_box"));
  l_bypass_button[1] = GTK_TOGGLE_BUTTON (lookup_widget (main_window, "mid_bypass"));
  comp_style[1] = gtk_widget_get_style (GTK_WIDGET (l_comp_bypass_event_box[1]));
  l_comp_color[1] = comp_style[1]->bg[GTK_STATE_NORMAL];

  l_comp_bypass_event_box[2] = GTK_EVENT_BOX (lookup_widget (main_window, "high_bypass_event_box"));
  l_bypass_button[2] = GTK_TOGGLE_BUTTON (lookup_widget (main_window, "high_bypass"));
  comp_style[2] = gtk_widget_get_style (GTK_WIDGET (l_comp_bypass_event_box[2]));
  l_comp_color[2] = comp_style[2]->bg[GTK_STATE_NORMAL];

  l_limiter_bypass_event_box = GTK_EVENT_BOX (lookup_widget (main_window, "limiter_bypass_event_box"));
  l_limiter_bypass = GTK_TOGGLE_BUTTON (lookup_widget (main_window, "limiter_bypass"));
  limiter_style = gtk_widget_get_style (GTK_WIDGET (l_limiter_bypass_event_box));
  l_limiter_color = limiter_style->bg[GTK_STATE_NORMAL];

  l_global_bypass_event_box = GTK_EVENT_BOX (lookup_widget (main_window, "global_bypass_event_box"));
  l_global_bypass = GTK_TOGGLE_BUTTON (lookup_widget (main_window, "global_bypass"));
  main_style = gtk_widget_get_style (GTK_WIDGET (l_global_bypass_event_box));
  l_main_color = main_style->bg[GTK_STATE_NORMAL];

  l_low_delay = GTK_TOGGLE_BUTTON (lookup_widget (main_window, "LowDelayButton"));
  l_mid_delay = GTK_TOGGLE_BUTTON (lookup_widget (main_window, "MidDelayButton"));

  scene_name_dialog = create_scene_name_dialog ();

  about_dialog = create_about_dialog ();

  l_scene_name_entry = GTK_ENTRY (lookup_widget (scene_name_dialog, "scene_name_entry"));
}


gboolean
on_EQ_curve_draw             (GtkWidget       *widget,
                                        cairo_t *cr,
                                        gpointer         data)
{
//    hdeq_curve_exposed (widget, event);
	hdeq_curve_draw (widget, cr, data);
    return FALSE;
}


void
on_EQ_curve_realize                    (GtkWidget       *widget,
                                        gpointer         user_data)
{
    hdeq_curve_init (widget);
}


gboolean 
on_EQ_curve_event_box_motion_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    hdeq_curve_motion (event);

    return FALSE;
}


gboolean
on_EQ_curve_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    hdeq_curve_button_press (event);

    return FALSE;
}


gboolean
on_EQ_curve_event_box_button_release_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    hdeq_curve_button_release (event);

    set_scene_warning_button ();

    return FALSE;
}


gboolean
on_EQ_curve_event_box_leave_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  int mode = process_get_spec_mode();

  switch (mode)
    {
    case SPEC_PRE_EQ:
      hdeq_curve_set_label (_("Pre EQ spectrum         "));
      break;
    case SPEC_POST_EQ:
      hdeq_curve_set_label (_("Post EQ spectrum        "));
      break;
    case SPEC_POST_COMP:
      hdeq_curve_set_label (_("Post compressor spectrum"));
      break;
    case SPEC_OUTPUT:
      hdeq_curve_set_label (_("Output spectrum         "));
      break;
    }

    return FALSE;
}


void
on_lim_out_trim_scale_value_changed        (GtkRange        *range,
                                        gpointer         user_data)
{
    s_set_value_ui(S_LIM_LIMIT,  
		    gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(range))));
}



gboolean
on_comp1_curve_draw            (GtkWidget       *widget,
                                        cairo_t *cr,
                                        gpointer         data)
{
   comp_curve_draw (widget, cr, 0);

    return FALSE;
}


void
on_comp1_curve_realize                 (GtkWidget       *widget,
                                        gpointer         user_data)
{
    comp_curve_realize (widget, 0);
}


gboolean
on_comp2_curve_draw            (GtkWidget       *widget,
                                        cairo_t *cr,
                                        gpointer         data)
{
    comp_curve_draw (widget, cr, 1);

    return FALSE;
}


void
on_comp2_curve_realize                 (GtkWidget       *widget,
                                        gpointer         user_data)
{
    comp_curve_realize (widget, 1);
}


gboolean
on_comp3_curve_draw            (GtkWidget       *widget,
                                        cairo_t *cr,
                                        gpointer         data)
{
    comp_curve_draw (widget, cr, 2);

    return FALSE;
}


void
on_comp3_curve_realize                 (GtkWidget       *widget,
                                        gpointer         user_data)
{
	comp_curve_realize (widget, 2);
}


gboolean
on_low_curve_box_motion_notify_event   (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    comp_curve_box_motion (0, event);

    return FALSE;
}


gboolean
on_mid_curve_box_motion_notify_event   (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    comp_curve_box_motion (1, event);

    return FALSE;
}


gboolean
on_high_curve_box_motion_notify_event  (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    comp_curve_box_motion (2, event);

    return FALSE;
}

gboolean
on_low_curve_box_leave_notify_event    (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    //draw_comp_curve (0);

    comp_box_leave (0);

    return FALSE;
}


gboolean
on_mid_curve_box_leave_notify_event    (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  //  draw_comp_curve (1);

    comp_box_leave (1);

    return FALSE;
}


gboolean
on_high_curve_box_leave_notify_event   (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  //  draw_comp_curve (2);

    comp_box_leave (2);

    return FALSE;
}

gboolean
on_low_curve_box_enter_notify_event    (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    comp_box_enter (0);

    return FALSE;
}


gboolean
on_mid_curve_box_enter_notify_event    (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    comp_box_enter (1);

    return FALSE;
}


gboolean
on_high_curve_box_enter_notify_event   (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    comp_box_enter (2);

    return FALSE;
}


gboolean
on_low_comp_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    comp_box_enter (0);

    return FALSE;
}


gboolean
on_low_comp_event_box_leave_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    comp_box_leave (0);

    return FALSE;
}


gboolean
on_mid_comp_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    comp_box_enter (1);

    return FALSE;
}


gboolean
on_mid_comp_event_box_leave_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    comp_box_leave (1);

    return FALSE;
}


gboolean
on_high_comp_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    comp_box_enter (2);

    return FALSE;
}


gboolean
on_high_comp_event_box_leave_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    comp_box_leave (2);

    return FALSE;
}


GtkWidget*
make_meter (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    GtkWidget *ret;
    gint dir = GTK_METER_UP;
    int sides = GTK_METERSCALE_TOP;
    GtkAdjustment *adjustment = (GtkAdjustment*) gtk_adjustment_new (0.0,
                    (float)int1, (float)int2, 0.0, 0.0, 0.0);

    if (!string1 || !strcmp(string1, "up")) {
        dir = GTK_METER_UP;
    } else if (!strcmp(string1, "down")) {
        dir = GTK_METER_DOWN;
    } else if (!strcmp(string1, "left")) {
        dir = GTK_METER_LEFT;
    } else if (!strcmp(string1, "right")) {
        dir = GTK_METER_RIGHT;
    }
  

    if (string1 && strstr(string1, "left")) {
        sides = GTK_METERSCALE_LEFT;
    }
    if (string1 && strstr(string1, "right")) {
        sides = GTK_METERSCALE_RIGHT;
    }
    if (string1 && strstr(string1, "top")) {
        sides = GTK_METERSCALE_TOP;
    }
    if (string1 && strstr(string1, "bottom")) {
        sides = GTK_METERSCALE_BOTTOM;
    }

    ret = gtk_meter_new(adjustment, dir, sides, int1, int2);
    gtk_meter_set_adjustment(GTK_METER (ret), adjustment);

    return ret;

  
}


GtkWidget*
make_mscale (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    int sides = 0;
    GtkWidget *ret;

 /*   if (string1 && strstr(string1, "left")) {
	sides |= GTK_METERSCALE_LEFT;
    }
    if (string1 && strstr(string1, "right")) {
	sides |= GTK_METERSCALE_RIGHT;
    }
    if (string1 && strstr(string1, "top")) {
	sides |= GTK_METERSCALE_TOP;
    }
    if (string1 && strstr(string1, "bottom")) {
	sides |= GTK_METERSCALE_BOTTOM;
    }

    ret = gtk_meterscale_new(sides, int1, int2);

    return ret;
    
*/ 
}


void
on_autobutton_1_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    comp_set_auto(0, gtk_toggle_button_get_active(togglebutton));
}


void
on_autobutton_2_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    comp_set_auto(1, gtk_toggle_button_get_active(togglebutton));
}


void
on_autobutton_3_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    comp_set_auto(2, gtk_toggle_button_get_active(togglebutton));
}


void
on_lim_lh_scale_value_changed          (GtkRange        *range,
                                        gpointer         user_data)
{
  s_set_value_ui(S_LIM_TIME,
                 gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(range))));
}

void
on_release_val_label_realize           (GtkWidget       *widget,
                                        gpointer         user_data)
{
    GtkRequisition size;

    gtk_widget_size_request(widget, &size);
 //   gtk_widget_set_usize(widget, size.width, -1);
}

void
on_hscale_1_l_value_changed               (GtkRange        *range,
                                        gpointer         user_data)
{
  s_set_value_ui(S_STEREO_WIDTH(0),
                 gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(range))));
}


void
on_hscale_1_l_realize                     (GtkWidget       *widget,
                                        gpointer         user_data)
{
  s_set_adjustment(S_STEREO_WIDTH(0),
                   gtk_range_get_adjustment(GTK_RANGE(widget)));
}


void
on_hscale_1_m_value_changed               (GtkRange        *range,
                                        gpointer         user_data)
{
  s_set_value_ui(S_STEREO_WIDTH(1),
                 gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(range))));
}


void
on_hscale_1_m_realize                     (GtkWidget       *widget,
                                        gpointer         user_data)
{
  s_set_adjustment(S_STEREO_WIDTH(1),
                   gtk_range_get_adjustment(GTK_RANGE(widget)));
}


void
on_hscale_1_h_value_changed               (GtkRange        *range,
                                        gpointer         user_data)
{
  s_set_value_ui(S_STEREO_WIDTH(2),
                 gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(range))));
}


void
on_hscale_1_h_realize                     (GtkWidget       *widget,
                                        gpointer         user_data)
{
  s_set_adjustment(S_STEREO_WIDTH(2),
                   gtk_range_get_adjustment(GTK_RANGE(widget)));
}


void
on_lim_input_hscale_value_changed      (GtkRange        *range,
                                        gpointer         user_data)
{
  s_set_value_ui(S_LIM_INPUT,
                 gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(range))));
}


void
on_lim_input_hscale_realize            (GtkWidget       *widget,
                                        gpointer         user_data)
{
  s_set_adjustment(S_LIM_INPUT,
                   gtk_range_get_adjustment(GTK_RANGE(widget)));
}


void
on_optionmenu1_realize                 (GtkWidget       *widget,
                                        gpointer         user_data)
{
//    gtk_option_menu_set_history (GTK_OPTION_MENU(widget), 1);
}



gboolean
backward_transport                     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    transport_skip(-5.0);
    return FALSE;
}


gboolean
forward_transport                      (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    transport_skip(5.0);
    return FALSE;
}


gboolean
pause_transport_toggle                 (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    transport_toggle_play();
    return FALSE;
}


gboolean
play_transport                         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    transport_play();
    return FALSE;
}


gboolean
rewind_transport                       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    transport_set_time(0.0);
    return FALSE;
}


gboolean
stop_transport                         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    transport_stop();
    return FALSE;
}


void
on_boost_scale_value_changed           (GtkRange        *range,
                                        gpointer         user_data)
{
    s_set_value_ui(S_BOOST,
                   gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(range))));
}


gboolean
on_scene1_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 0;
  select_scene (0, event->button);

  return FALSE;
}


gboolean
on_scene2_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 1;
  select_scene (1, event->button);

  return FALSE;
}


gboolean
on_scene3_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 2;
  select_scene (2, event->button);

  return FALSE;
}


gboolean
on_scene4_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 3;
  select_scene (3, event->button);

  return FALSE;
}


gboolean
on_scene5_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 4;
  select_scene (4, event->button);

  return FALSE;
}


gboolean
on_scene6_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 5;
  select_scene (5, event->button);

  return FALSE;
}


gboolean
on_scene7_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 6;
  select_scene (6, event->button);

  return FALSE;
}


gboolean
on_scene8_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 7;
  select_scene (7, event->button);

  return FALSE;
}


gboolean
on_scene9_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 8;
  select_scene (8, event->button);

  return FALSE;
}


gboolean
on_scene10_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 9;
  select_scene (9, event->button);

  return FALSE;
}


gboolean
on_scene11_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 10;
  select_scene (10, event->button);

  return FALSE;
}


gboolean
on_scene12_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 11;
  select_scene (11, event->button);

  return FALSE;
}


gboolean
on_scene13_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 12;
  select_scene (12, event->button);

  return FALSE;
}


gboolean
on_scene14_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 13;
  select_scene (13, event->button);

  return FALSE;
}


gboolean
on_scene15_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 14;
  select_scene (14, event->button);

  return FALSE;
}


gboolean
on_scene16_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 15;
  select_scene (15, event->button);

  return FALSE;
}


gboolean
on_scene17_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 16;
  select_scene (16, event->button);

  return FALSE;
}


gboolean
on_scene18_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 17;
  select_scene (17, event->button);

  return FALSE;
}


gboolean
on_scene19_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 18;
  select_scene (18, event->button);

  return FALSE;
}


gboolean
on_scene20_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  hot_scene = 19;
  select_scene (19, event->button);

  return FALSE;
}


void
on_setscene_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    set_scene (-1);
}


void
on_clearscene_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    clear_scene (-1);
}


gboolean
on_EQ_curve_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  //  help_ptr = _(hdeq_help);

    return FALSE;
}


gboolean
on_show_help                           (GtkWidget       *widget,
                                        GtkWidgetHelpType  help_type,
                                        gpointer         user_data)
{
    message (GTK_MESSAGE_INFO, help_ptr);

    return FALSE;
}


gboolean
on_input_eventbox_enter_notify_event   (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    help_ptr = _(input_help);

    return FALSE;
}

gboolean
on_geq_eventbox_enter_notify_event     (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    help_ptr = _(geq_help);

    return FALSE;
}


gboolean
on_spectrum_eventbox_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    help_ptr = _(spectrum_help);

    return FALSE;
}


gboolean
on_crossover_eventbox_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    help_ptr = _(crossover_help);

    return FALSE;
}


gboolean
on_comp_curve_eventbox_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
 //   help_ptr = _(comp_curve_help);

    return FALSE;
}


gboolean
on_limiter_eventbox_enter_notify_event (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    help_ptr = _(limiter_help);

    return FALSE;
}


gboolean
on_boost_eventbox_enter_notify_event   (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    help_ptr = _(boost_help);

    return FALSE;
}


gboolean
on_output_eventbox_enter_notify_event  (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    help_ptr = _(output_help);

    return FALSE;
}

gboolean
on_help_button_enter_notify_event      (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    help_ptr = _(general_help);

    return FALSE;
}


gboolean
on_eq_options_eventbox_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    force_keypress_help = TRUE;
    help_ptr = _(eq_options_help);

    return FALSE;
}


gboolean
on_spectrum_options_eventbox_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    help_ptr = _(spectrum_options_help);

    return FALSE;
}


gboolean
on_transport_controls_eventbox_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    help_ptr = _(transport_controls_help);

    return FALSE;
}


gboolean
on_scenes_eventbox_enter_notify_event  (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
    help_ptr = _(scenes_help);

    return FALSE;
}


void
on_notebook1_switch_page               (GtkNotebook     *notebook,
                                        GtkWidget *page,
                                        guint            page_num,
                                        gpointer         user_data)
{
    hdeq_notebook1_set_page (page_num);
}


void
on_new1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  s_load_session(NULL);

  unset_scene_buttons ();
  set_scene_button (0);


  reset_hdeq ();
}


void
on_open1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
#if GTK_VERSION_GE(2, 4)

    GtkFileChooser *file_selector;
    GtkFileFilter *filter = gtk_file_filter_new ();

    file_selector = (GtkFileChooser *) gtk_file_chooser_dialog_new (
      _("Select a session file"),
      NULL,
      GTK_FILE_CHOOSER_ACTION_OPEN,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
      NULL
    );

    if (jamin_dir) {
        gtk_file_chooser_set_current_folder (file_selector, jamin_dir);
    }

    gtk_file_filter_add_pattern (filter, "*.jam");
    gtk_file_chooser_set_filter (file_selector, filter);

    if (gtk_dialog_run (GTK_DIALOG (file_selector)) == GTK_RESPONSE_ACCEPT) {
        s_load_session_from_ui (NULL, (gpointer) file_selector);
    }

    gtk_widget_destroy (GTK_WIDGET (file_selector));

#else

    GtkFileSelection    *file_selector;

    file_selector = 
      (GtkFileSelection *) gtk_file_selection_new (_("Select a session file"));

    if (jamin_dir) {
        gtk_file_selection_set_filename (file_selector, jamin_dir);
    }

    gtk_file_selection_complete (file_selector, "*.jam");

    g_signal_connect (GTK_OBJECT (file_selector->ok_button),
        "clicked", G_CALLBACK (s_load_session_from_ui), file_selector);

    g_signal_connect_swapped (GTK_OBJECT (file_selector->ok_button),
        "clicked", G_CALLBACK (gtk_widget_destroy), (gpointer) file_selector);

    g_signal_connect_swapped (GTK_OBJECT (file_selector->cancel_button),
        "clicked", G_CALLBACK (gtk_widget_destroy), (gpointer) file_selector);

    gtk_widget_show ((GtkWidget *) file_selector);

#endif
}


void
on_save_as1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gchar *fname = NULL;

    if (s_have_session_filename ()) {
        fname = s_get_session_filename ();
    }

#if GTK_VERSION_GE(2, 4)

    GtkFileChooser *file_selector;

    file_selector = (GtkFileChooser *) gtk_file_chooser_dialog_new (
      _("Select a session file"),
      NULL,
      GTK_FILE_CHOOSER_ACTION_SAVE,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
      NULL
    );

    if (jamin_dir) {
        gtk_file_chooser_set_current_folder (file_selector, jamin_dir);
    }

    if (fname != NULL) {
        gtk_file_chooser_set_current_name (file_selector, fname);
    } else {
        gtk_file_chooser_set_current_name (file_selector, "default.jam");
    }

    if (gtk_dialog_run (GTK_DIALOG (file_selector)) == GTK_RESPONSE_ACCEPT) {
        s_save_session_from_ui (NULL, (gpointer) file_selector);
    }

    gtk_widget_destroy (GTK_WIDGET (file_selector));

#else

    GtkFileSelection    *file_selector;

    file_selector = 
      (GtkFileSelection *) gtk_file_selection_new (_("Select a session file"));

    if (fname != NULL)
      {
	gtk_file_selection_set_filename (file_selector, fname);
      }
    else
      {
        if (jamin_dir) 
            gtk_file_selection_set_filename (file_selector, jamin_dir);
        gtk_file_selection_complete (file_selector, "default.jam");
      }

    g_signal_connect (GTK_OBJECT (file_selector->ok_button),
        "clicked", G_CALLBACK (s_save_session_from_ui), file_selector);

    g_signal_connect_swapped (GTK_OBJECT (file_selector->ok_button),
        "clicked", G_CALLBACK (gtk_widget_destroy), (gpointer) file_selector);

    g_signal_connect_swapped (GTK_OBJECT (file_selector->cancel_button),
        "clicked", G_CALLBACK (gtk_widget_destroy), (gpointer) file_selector);

    gtk_widget_show ((GtkWidget *) file_selector);

#endif
}


void
on_save1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	if (s_have_session_filename ()) {
		s_save_session (NULL);
	} else {
          on_save_as1_activate (NULL, NULL);
	}
}


void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    clean_quit();
}


void
on_undo1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  s_undo();
}

void
on_redo1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  s_redo();
}


void general_help_callback ()
{
  message (GTK_MESSAGE_INFO, _(general_help));
}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gtk_widget_show (about_dialog);
}


void
on_about_prerequisites1_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  message (GTK_MESSAGE_INFO, _(prerequisites_help));
}


gboolean
on_frame_l_enter_notify_event          (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  comp_box_enter (0);

  return FALSE;
}


gboolean
on_frame_m_enter_notify_event          (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  comp_box_enter (1);

  return FALSE;
}


gboolean
on_frame_h_enter_notify_event          (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  comp_box_enter (2);

  return FALSE;
}


void
on_help1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  message (GTK_MESSAGE_INFO, _(help_help));
}


void
on_keys1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  message (GTK_MESSAGE_INFO, _(keys_help));
}


gboolean
on_window1_key_press_event             (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    gboolean              tmp;
    unsigned int          key, state;
    int                   scene;


    key = event->keyval;


    /*  Trying to check for Num Lock and/or other weird modifiers that I 
        don't know about.  */

    state = event->state;
    if (state & GDK_LOCK_MASK) state &= ~GDK_LOCK_MASK;
    if (state & GDK_MOD2_MASK) state &= ~GDK_MOD2_MASK;
    if (state & GDK_MOD3_MASK) state &= ~GDK_MOD3_MASK;
    if (state & GDK_MOD4_MASK) state &= ~GDK_MOD4_MASK;
    if (state & GDK_MOD5_MASK) state &= ~GDK_MOD5_MASK;


    /*  Force context help for EQ Options and Preferences dialogs.  This fixes
        an initial focus problem with the dialogs.  There must be a better way
        to do it but I don't know what it is.   JCD  */

    if (force_keypress_help)
      {
        if (key == GDK_KEY_F1 && state == GDK_SHIFT_MASK)   
          message (GTK_MESSAGE_INFO, help_ptr);
	force_keypress_help = FALSE;
      }


    /*  If a text widget has the focus we don't want to trap key presses.  */

    if (text_focus) return FALSE;


    scene = -1;


    switch (key)
      {

        /*  Bypass  */

      case GDK_KEY_b:
        tmp = gtk_toggle_button_get_active (l_global_bypass);
        gtk_toggle_button_set_active (l_global_bypass, (!tmp));
        break;


        /*  Toggle play  */

      case GDK_KEY_space:
	transport_toggle_play();
        break;


        /*  Rewind  */

      case GDK_KEY_Home:
	transport_set_time(0.0);
        break;

	/*  Skip backward */

      case GDK_KEY_less:
	transport_skip(-5.0);
        break;


        /*  Skip forward  */

      case GDK_KEY_greater:
	transport_skip(5.0);
        break;


        /*  Select scene 1  */

      case GDK_KEY_1:
      case GDK_KEY_KP_1:
        scene = 0;
        break;


        /*  Select scene 2  */

      case GDK_KEY_2:
      case GDK_KEY_KP_2:
        scene = 1;
        break;


        /*  Select scene 3  */

      case GDK_KEY_3:
      case GDK_KEY_KP_3:
        scene = 2;
        break;


        /*  Select scene 4  */

      case GDK_KEY_4:
      case GDK_KEY_KP_4:
        scene = 3;
        break;


        /*  Select scene 5  */

      case GDK_KEY_5:
      case GDK_KEY_KP_5:
        scene = 4;
        break;


        /*  Select scene 6  */

      case GDK_KEY_6:
      case GDK_KEY_KP_6:
        scene = 5;
        break;

 
        /*  Select scene 7  */

      case GDK_KEY_7:
      case GDK_KEY_KP_7:
        scene = 6;
        break;

 
        /*  Select scene 8  */

      case GDK_KEY_8:
      case GDK_KEY_KP_8:
        scene = 7;
        break;

 
        /*  Select scene 9  */

      case GDK_KEY_9:
      case GDK_KEY_KP_9:
        scene = 8;
        break;

 
        /*  Select scene 10  */

      case GDK_KEY_0:
      case GDK_KEY_KP_0:
        scene = 9;
        break;

 
        /*  Select scene 11  */

      case GDK_KEY_exclam:
        scene = 10;
        break;


        /*  Select scene 12  */

      case GDK_KEY_at:
        scene = 11;
        break;


        /*  Select scene 13  */

      case GDK_KEY_numbersign:
        scene = 12;
        break;


        /*  Select scene 14  */

      case GDK_KEY_dollar:
        scene = 13;
        break;


        /*  Select scene 15  */

      case GDK_KEY_percent:
        scene = 14;
        break;


        /*  Select scene 16  */

      case GDK_KEY_asciicircum:
        scene = 15;
        break;


        /*  Select scene 17  */

      case GDK_KEY_ampersand:
        scene = 16;
        break;


        /*  Select scene 18  */

      case GDK_KEY_asterisk:
        scene = 17;
        break;


        /*  Select scene 19  */

      case GDK_KEY_parenleft:
        scene = 18;
        break;


        /*  Select scene 20  */

      case GDK_KEY_parenright:
        scene = 19;
        break;


        /*  Switch to tab 1 (HDEQ) unless we're getting help.  */

      case GDK_KEY_F1:
        if (state != GDK_SHIFT_MASK) 
            gtk_notebook_set_current_page (l_notebook1, 0);
        break;


        /*  Switch to tab 2 (30 band EQ)  */

      case GDK_KEY_F2:
        gtk_notebook_set_current_page (l_notebook1, 1);
        break;


        /*  Switch to tab 3 (Spectrum)  */

      case GDK_KEY_F3:
        gtk_notebook_set_current_page (l_notebook1, 2);
        break;


        /*  Switch to tab 4 (Compressor curves)  */

      case GDK_KEY_F4:
        gtk_notebook_set_current_page (l_notebook1, 3);
        break;


        /*  Save As session  */

      case GDK_KEY_a:
        if (state == GDK_CONTROL_MASK) on_save_as1_activate (NULL, NULL);
        break;
      }


    /*  Check modifiers for scene set and clear.  */

    if (scene >= 0)
      {
        switch (state)
          {
          case 0:
          case GDK_SHIFT_MASK:
            select_scene (scene, 1);
            break;

          case GDK_MOD1_MASK:
          case (GDK_MOD1_MASK | GDK_SHIFT_MASK):
            set_scene (scene);
            break;

          case GDK_CONTROL_MASK:
          case (GDK_CONTROL_MASK | GDK_SHIFT_MASK):
            clear_scene (scene);
            break;

          }
      }

    return FALSE;
}


/* JACK Ports menubar pulldown */

void
on_jack_ports_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  iomenu_pull_down_ports(menuitem);
}


gboolean
scene_warning                          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  set_scene_warning_button ();

  return FALSE;
}


gboolean
on_comp_event_box_enter_notify_event (GtkWidget       *widget,
                                      GdkEventCrossing *event,
                                      gpointer         user_data)
{
  help_ptr = _(comp_help);

  return FALSE;
}


gboolean
on_stereo_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  help_ptr = _(stereo_help);

  return FALSE;
}


gboolean
reset_range                            (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  set_scene_warning_button ();

  if (event->button == 3) gtk_range_set_value ((GtkRange *) widget, 0.0);

  return FALSE;
}


gboolean
on_comp_at_label_1_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_at (0);

  return FALSE;
}


gboolean
on_comp_re_label_1_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_re (0);

  return FALSE;
}


gboolean
on_comp_th_label_1_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_th (0);

  return FALSE;
}


gboolean
on_comp_ra_label_1_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_ra (0);

  return FALSE;
}


gboolean
on_comp_kn_label_1_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_kn (0);

  return FALSE;
}


gboolean
on_comp_ma_label_1_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_ma (0);

  return FALSE;
}


gboolean
on_comp_at_label_2_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_at (1);

  return FALSE;
}


gboolean
on_comp_re_label_2_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_re (1);

  return FALSE;
}


gboolean
on_comp_th_label_2_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_th (1);

  return FALSE;
}


gboolean
on_comp_ra_label_2_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_ra (1);

  return FALSE;
}


gboolean
on_comp_kn_label_2_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_kn (1);

  return FALSE;
}


gboolean
on_comp_ma_label_2_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_ma (1);

  return FALSE;
}


gboolean
on_comp_at_label_3_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_at (2);

  return FALSE;
}


gboolean
on_comp_re_label_3_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_re (2);

  return FALSE;
}


gboolean
on_comp_th_label_3_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_th (2);

  return FALSE;
}


gboolean
on_comp_ra_label_3_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_ra (2);

  return FALSE;
}


gboolean
on_comp_kn_label_3_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_kn (2);

  return FALSE;
}


gboolean
on_comp_ma_label_3_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  comp_gang_ma (2);

  return FALSE;
}


void
on_solo_toggled                    (GtkToggleButton *togglebutton,
                                    gpointer         user_data)
{
  int       i;
  gboolean solo;


  /*  Check for ANY soloed band.  */

  solo = FALSE;
  for (i = 0 ; i < XO_NBANDS ; i++)
    {
      if (gtk_toggle_button_get_active (l_solo_button[i])) solo = TRUE;
    }


  /*  If a band has been soloed...  */

  if (solo)
    {

      /*  Check each band's solo and bypass buttons.  */

      for (i = 0 ; i < XO_NBANDS ; i++)
        {

          /*  If it's soloed...  */

          if (gtk_toggle_button_get_active (l_solo_button[i]))
            {

              /*  If it's bypassed set it to BYPASS otherwise set it 
                  ACTIVE.  */

              if (gtk_toggle_button_get_active (l_bypass_button[i]))
                {
                  process_set_xo_band_action (i, BYPASS);
                }
              else
                {
                  process_set_xo_band_action (i, ACTIVE);
                }
            }

          /*  If it's not soloed, MUTE it.  */

          else
            {
              process_set_xo_band_action (i, MUTE);
            }
        }
    }

  /*  If no bands have been soloed then check all band bypass buttons and 
      set to either BYPASS or ACTIVE.  */

  else
    {
      for (i = 0 ; i < XO_NBANDS ; i++)
        {
          if (gtk_toggle_button_get_active (l_bypass_button[i]))
            {
              process_set_xo_band_action (i, BYPASS);
            }
          else
            {
              process_set_xo_band_action (i, ACTIVE);
            }
        }
    }
}


void
bypass (int band, gboolean toggled)
{
  int       i;
  gboolean solo;


  /*  Check for ANY soloed band.  */

  solo = FALSE;
  for (i = 0 ; i < XO_NBANDS ; i++)
    {
      if (gtk_toggle_button_get_active (l_solo_button[i])) solo = TRUE;
    }


  /*  We only want to set BYPASS or ACTIVE if no bands have been soloed or if
      this band has been soloed.  Otherwise we just leave the buttons as they 
      are since, if this band is not soloed and another is, this band needs to
      remain MUTE.  Whenever the next solo change happens (the only thing that
      can unmute this band) we'll set this band to BYPASS or ACTIVE based on 
      the settings of it's buttons.  */

  if (!solo || gtk_toggle_button_get_active (l_solo_button[band]))
    {
      if (toggled)
        {
          process_set_xo_band_action (band, BYPASS);
        }
      else
        {
          process_set_xo_band_action (band, ACTIVE);
        }
    }
}


void
on_low_bypass_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  int state;


  state = gtk_toggle_button_get_active(togglebutton);

  bypass (0, state);


  if (state) 
    {
      callbacks_blink_bypass_button (LOW_COMP_BYPASS, 1);
    }
  else
    {
      callbacks_blink_bypass_button (LOW_COMP_BYPASS, -1);
    }

}


void
on_mid_bypass_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  int state;


  state = gtk_toggle_button_get_active(togglebutton);

  bypass (1, state);


  if (state) 
    {
      callbacks_blink_bypass_button (MID_COMP_BYPASS, 1);
    }
  else
    {
      callbacks_blink_bypass_button (MID_COMP_BYPASS, -1);
    }
}


void
on_high_bypass_toggled                  (GtkToggleButton *togglebutton,
                                         gpointer         user_data)
{
  int state;


  state = gtk_toggle_button_get_active(togglebutton);

  bypass (2, state);


  if (state) 
    {
      callbacks_blink_bypass_button (HIGH_COMP_BYPASS, 1);
    }
  else
    {
      callbacks_blink_bypass_button (HIGH_COMP_BYPASS, -1);
    }
}


void 
callbacks_set_comp_bypass_button_state (int band, int state)
{
  gtk_toggle_button_set_active (l_bypass_button[band], state);
}


void 
callbacks_set_eq_bypass_button_state (int state)
{
  gtk_toggle_button_set_active (l_eq_bypass, state);
}


void 
callbacks_set_limiter_bypass_button_state (int state)
{
  gtk_toggle_button_set_active (l_limiter_bypass, state);
}


void 
callbacks_set_low_delay_button_state (int state)
{
  gtk_toggle_button_set_active (l_low_delay, state);
}


void 
callbacks_set_mid_delay_button_state (int state)
{
  gtk_toggle_button_set_active (l_mid_delay, state);
}


gboolean
on_eq_bypass_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  help_ptr = _(eq_bypass_help);

  return FALSE;
}


void
on_eq_bypass_toggled                   (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  int state;


  state = gtk_toggle_button_get_active(togglebutton);

  process_set_eq_bypass(state);


  if (state) 
    {
      callbacks_blink_bypass_button (EQ_BYPASS, 1);
    }
  else
    {
      callbacks_blink_bypass_button (EQ_BYPASS, -1);
    }
}


gboolean
on_band_button_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  help_ptr = _(band_button_help);

  return FALSE;
}


void
on_limiter_bypass_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  int state;


  state = gtk_toggle_button_get_active(togglebutton);

  process_set_limiter_bypass(state);


  if (state) 
    {
      callbacks_blink_bypass_button (LIMITER_BYPASS, 1);
    }
  else
    {
      callbacks_blink_bypass_button (LIMITER_BYPASS, -1);
    }
}


gboolean
on_limiter_bypass_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  help_ptr = _(limiter_bypass_help);

  return FALSE;
}


/*  Pop up the scene name dialog.  */

void popup_scene_name_dialog (int updown)
{
  if (updown)
    {
      gtk_widget_show (scene_name_dialog);
    }
  else
    {
      gtk_widget_hide (scene_name_dialog);
    }
}


void
on_name_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gtk_window_set_title (GTK_WINDOW (scene_name_dialog), 
                        get_scene_name (hot_scene));

  popup_scene_name_dialog (1);
}


void
on_scene_name_entry_changed            (GtkEditable     *editable,
                                        gpointer         user_data)
{
  strcpy (scene_name_text, gtk_entry_get_text (l_scene_name_entry));
}


void
on_scene_name_cancel_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
  popup_scene_name_dialog (0);
}


void
on_scene_name_ok_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
  set_scene_name (hot_scene, scene_name_text);
  popup_scene_name_dialog (0);
}


void
on_ft_bias_a_value_changed             (GtkRange        *range,
                                        gpointer         user_data)
{
#ifdef FILTER_TUNING
  ft_bias_a_val = gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(range)));
#endif
}


void
on_ft_bias_b_value_changed             (GtkRange        *range,
                                        gpointer         user_data)
{
#ifdef FILTER_TUNING
  ft_bias_b_val = gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(range)));
#endif
}


void
on_ft_rez_lp_a_value_changed           (GtkRange        *range,
                                        gpointer         user_data)
{
#ifdef FILTER_TUNING
  ft_rez_lp_a_val = gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(range)));
#endif
}


void
on_ft_rez_hp_a_value_changed           (GtkRange        *range,
                                        gpointer         user_data)
{
#ifdef FILTER_TUNING
  ft_rez_hp_a_val = gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(range)));
#endif
}


void
on_ft_rez_lp_b_value_changed           (GtkRange        *range,
                                        gpointer         user_data)
{
#ifdef FILTER_TUNING
  ft_rez_lp_b_val = gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(range)));
#endif
}


void
on_ft_rez_hp_b_value_changed           (GtkRange        *range,
                                        gpointer         user_data)
{
#ifdef FILTER_TUNING
  ft_rez_hp_b_val = gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(range)));
#endif
}


void
on_ft_bias_a_hp_value_changed          (GtkRange        *range,
                                        gpointer         user_data)
{
#ifdef FILTER_TUNING
  ft_bias_a_hp_val = gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(range)));
#endif
}


void
on_ft_bias_b_hp_value_changed          (GtkRange        *range,
                                        gpointer         user_data)
{
#ifdef FILTER_TUNING
  ft_bias_b_hp_val = gtk_adjustment_get_value(gtk_range_get_adjustment(GTK_RANGE(range)));
#endif
}


void
on_about_closebutton_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_hide (about_dialog);
}


void
on_MinGainSpin_value_changed           (GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
    float gain;


    gain = gtk_spin_button_get_value (spinbutton);

    hdeq_set_lower_gain (gain);

    geq_set_range (gain, gtk_adjustment_get_upper(geq_get_adjustment(0)));
}


void
on_MaxGainSpin_value_changed           (GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
    float gain;


    gain = gtk_spin_button_get_value (spinbutton);

    hdeq_set_upper_gain (gain);

    geq_set_range (gtk_adjustment_get_lower(geq_get_adjustment(0)), gain);
}


void
on_CrossfadeTimeSpin_value_changed     (GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
  float ct = gtk_spin_button_get_value (spinbutton);

  s_set_crossfade_time (ct);
}


void
on_UpdateFrequencySpin_value_changed   (GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
  int freq = gtk_spin_button_get_value (spinbutton);

  set_spectrum_freq (freq);
}


void
on_preferences1_activate               (GtkMenuItem     *menuitem,
				        gpointer         user_data)
{
  popup_pref_dialog (1);
}


void
on_pref_close_clicked            (GtkButton       *button,
				  gpointer         user_data)
{
  popup_pref_dialog (0);
}


gboolean
on_text_focus_in_event                 (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
  text_focus = TRUE;

  return FALSE;
}


gboolean
on_text_focus_out_event                (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
  text_focus = FALSE;

  return FALSE;
}


void
on_options1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
}


gboolean
on_pref_enter_notify_event             (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  force_keypress_help = TRUE;
  help_ptr = _(preferences_help);

  return FALSE;
}

void
on_FFTButton_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
  process_set_crossover_type (FFT);
}


void
on_IIRButton_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
  process_set_crossover_type (IIR);
}


gboolean
on_inmeter_eventbox_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  if (event->button == 3) intrim_inmeter_reset_peak ();

  return FALSE;
}


gboolean
on_outmeter_eventbox_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  if (event->button == 3)
    {
      intrim_outmeter_reset_peak ();
      intrim_rmsmeter_reset_peak ();
    }

  return FALSE;
}


gboolean
on_lim_in_meter_eventbox_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  if (event->button == 3) limiter_inmeter_reset_peak ();

  return FALSE;
}


gboolean
on_lim_out_meter_eventbox_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  if (event->button == 3) limiter_outmeter_reset_peak ();

  return FALSE;
}


void
on_warningLevelSpinButton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
  float wl = gtk_spin_button_get_value (spinbutton);

  intrim_inmeter_set_warn (wl);
  intrim_outmeter_set_warn (wl);
}


  /*  I've included the following helpful information that I found on the web 
      since this is an extremely confusing thing.  I'm passing the name of a
      widget as the "Object" with the signal.  You would expect that to show
      up as user_data but as you can see from the following, it doesn't ;-)
      JCD 04/25/2007


      Hi Ashwin,

      you've just outlined one of the problems that have existed in glade/libglade
      for a long time, let me try to illustrate how it works and what we've done
      to address that.

      The way it works:
      Currently if you set the "object" string in the signal handler, libglade
      will use that to lookup the said "object" by name in the interface
      (i.e. the same widget that would be returned by glade_xml_get_widget () on
      that string would become your userdata)... but wait ! it gets even more confusing,
      signals in glade files that bear the "object" attribute are always connected
      with the SWAPPED flag, that means that the "object" will be passed to your
      callback as the _first_ argument and the actual object that emitted the signal
      in question will be passed as the user_data argument to the signal handler.

      One step to improve this:
      in glade3 we originally included a "lookup" checkbutton on the signal
      data and supplied support for this in libglade as part of a GObject support
      patch to libglade: http://bugzilla.gnome.org/show_bug.cgi?id=161903 .
      this approach; if ever applied to libglade (or the future gtk+ builder code)
      would allow us to specify a symbol name for the user_data argument, the
      builder code (i.e. libglade) would simply g_module_lookup() the symbol
      string provided and pass that address to the callback as user_data.

      Ideally it would probably be great to allow the user to specify enum/flag/int
      values to user_data as well, enums and flags would be difficult because they
      have to be typed.

      Discussion on improving this situation probably belongs on gtk-devel-list
      or on http://bugzilla.gnome.org/show_bug.cgi?id=172535 ...

      Cheers,
                     -Tristan
  */

/*  Passing the band label from under the band slider as "Object" in glade-2.  The
    label becomes *range and the range becomes user_data - weird.  */

void
on_eqb_value_changed                   (GtkRange        *range,
                                        gpointer         user_data)
{
  char *freq = NULL;
  char *amp = NULL;
  int th, hu, val;

  th = 0;
  hu = 0;
  val = 0;
  if (strchr (gtk_label_get_text (GTK_LABEL (range)), 'k'))
    {
      sscanf (gtk_label_get_text (GTK_LABEL (range)), "%dk%d", &th, &hu);
      val = th * 1000 + hu * 100;
    }
  else
    {
      sscanf (gtk_label_get_text (GTK_LABEL (range)), "%d", &val);
    }

  freq = g_strdup_printf ("<b>%d Hz</b>", val);
  amp = g_strdup_printf ("<b>%.1f dB</b>", gtk_range_get_value (GTK_RANGE (user_data)));

  gtk_label_set_label (l_eqbFreqLabel, freq);
  gtk_label_set_label (l_eqbAmpLabel, amp);

  free (freq);
  free (amp);
}


/*  Passing the band slider as "Object" in glade-2.  The slider becomes *widget
    and the event box widget becomes user_data.  */

gboolean
on_eqb_enter_notify_event              (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  char *amp = NULL;

  amp = g_strdup_printf ("<b>%.1f dB</b>", gtk_range_get_value (GTK_RANGE (widget)));

  gtk_label_set_label (l_eqbAmpLabel, amp);

  free (amp);

  return FALSE;
}


/*  Passing the band label from under the band slider as "Object" in glade-2.  The
    label becomes *widget and the event box becomes user_data.  */

gboolean
on_eqbl_enter_notify_event             (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  char *freq = NULL;
  int th, hu, val;

  th = 0;
  hu = 0;
  val = 0;
  if (strchr (gtk_label_get_text (GTK_LABEL (widget)), 'k'))
    {
      sscanf (gtk_label_get_text (GTK_LABEL (widget)), "%dk%d", &th, &hu);
      val = th * 1000 + hu * 100;
    }
  else
    {
      sscanf (gtk_label_get_text (GTK_LABEL (widget)), "%d", &val);
    }

  freq = g_strdup_printf ("<b>%d Hz</b>", val);

  gtk_label_set_label (l_eqbFreqLabel, freq);

  free (freq);

  return FALSE;
}


void
on_global_bypass_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  int state;
  GtkToggleButton *togglebutton1;

  state = gtk_toggle_button_get_active(togglebutton);

  process_set_global_bypass (state);

  if ( togglebutton == lookup_widget(presets_window, "checkbutton1")) 
  {
	  togglebutton1 = lookup_widget(main_window, "global_bypass");
	  
  } else {
	  togglebutton1 = lookup_widget(presets_window, "checkbutton1");
	  
  }

  if (state) 
    {
      callbacks_blink_bypass_button (GLOBAL_BYPASS, 1);
      gtk_toggle_button_set_active(togglebutton1, TRUE);
    }
  else
    {
      callbacks_blink_bypass_button (GLOBAL_BYPASS, -1);
      gtk_toggle_button_set_active(togglebutton1, FALSE);
    }
}



/*  start = -1 sets the button to the background color, 1 starts it as red, 0 flips it
    between red and background.  */

void 
callbacks_blink_bypass_button (int button, int start)
{
  static int eq_flip, comp_flip[3], limiter_flip, global_flip;
  GdkColor    bypass;


  switch (button)
    {
    case EQ_BYPASS:
      if (start < 0)
        {
          bypass = l_eq_color;
        }
      else
        {
          if (start) eq_flip = 1;


          if (eq_flip)
            {
              bypass.red = 65535;
              bypass.green = 0;
              bypass.blue = 0;
            }
          else
            {
              bypass = l_eq_color;
            }
          eq_flip ^= 1;
        }

      gtk_widget_modify_bg ((GtkWidget *) l_eq_bypass_event_box, GTK_STATE_NORMAL, &bypass);
      break;

    case LOW_COMP_BYPASS:
      if (start < 0)
        {
          bypass = l_comp_color[0];
        }
      else
        {
          if (start) comp_flip[0] = 1;


          if (comp_flip[0])
            {
              bypass.red = 65535;
              bypass.green = 0;
              bypass.blue = 0;
            }
          else
            {
              bypass = l_comp_color[0];
            }
          comp_flip[0] ^= 1;
        }

      gtk_widget_modify_bg ((GtkWidget *) l_comp_bypass_event_box[0], GTK_STATE_NORMAL, &bypass);
      break;

    case MID_COMP_BYPASS:
      if (start < 0)
        {
          bypass = l_comp_color[1];
        }
      else
        {
          if (start) comp_flip[1] = 1;


          if (comp_flip[1])
            {
              bypass.red = 65535;
              bypass.green = 0;
              bypass.blue = 0;
            }
          else
            {
              bypass = l_comp_color[1];
            }
          comp_flip[1] ^= 1;
        }

      gtk_widget_modify_bg ((GtkWidget *) l_comp_bypass_event_box[1], GTK_STATE_NORMAL, &bypass);
      break;

    case HIGH_COMP_BYPASS:
      if (start < 0)
        {
          bypass = l_comp_color[2];
        }
      else
        {
          if (start) comp_flip[2] = 1;


          if (comp_flip[2])
            {
              bypass.red = 65535;
              bypass.green = 0;
              bypass.blue = 0;
            }
          else
            {
              bypass = l_comp_color[2];
            }
          comp_flip[2] ^= 1;
        }

      gtk_widget_modify_bg ((GtkWidget *) l_comp_bypass_event_box[2], GTK_STATE_NORMAL, &bypass);
      break;

    case LIMITER_BYPASS:
      if (start < 0)
        {
          bypass = l_limiter_color;
        }
      else
        {
          if (start) limiter_flip = 1;


          if (limiter_flip)
            {
              bypass.red = 65535;
              bypass.green = 0;
              bypass.blue = 0;
            }
          else
            {
              bypass = l_limiter_color;
            }
          limiter_flip ^= 1;
        }

      gtk_widget_modify_bg ((GtkWidget *) l_limiter_bypass_event_box, GTK_STATE_NORMAL, &bypass);
      break;

    case GLOBAL_BYPASS:
      if (start < 0)
        {
          bypass = l_main_color;
        }
      else
        {
          if (start) global_flip = 1;


          if (global_flip)
            {
              bypass.red = 65535;
              bypass.green = 0;
              bypass.blue = 0;
            }
          else
            {
              bypass = l_main_color;
            }
          global_flip ^= 1;
        }

      gtk_widget_modify_bg ((GtkWidget *) l_global_bypass_event_box, GTK_STATE_NORMAL, &bypass);
      gtk_widget_modify_bg (lookup_widget(presets_window, "global_bypass_event_box_presets"), GTK_STATE_NORMAL, &bypass);
      break;
    }
} 


gboolean
on_global_bypass_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  help_ptr = _(bypass_help);

  return FALSE;
}


/*  Passing the gtk_meter as "Object" in glade-2.  The gtk_meter becomes
    *widget and the text field becomes user_data.  */

gboolean
on_meter_text_button_press_event (GtkWidget       *widget,
                                  GdkEventButton  *event,
                                  gpointer         user_data)
{
 // gtk_meter_reset_peak ((GtkMeter *) widget);

  return FALSE;
}

void
on_out_meter_peak_button_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
  intrim_set_out_meter_peak_pref (TRUE);
}


void
on_out_meter_full_button_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
  intrim_set_out_meter_peak_pref (FALSE);
}


void
on_rms_meter_peak_button_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
  intrim_set_rms_meter_peak_pref (TRUE);
}


void
on_rms_meter_full_button_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
  intrim_set_rms_meter_peak_pref (FALSE);
}


/*  Passing rmsSamples as "Object" in glade-2.  This becomes *spinbutton
    and the spinbutton (rmsTime) becomes user_data (see above comments).  */

void
on_rmsTimeValue_value_changed  (GtkSpinButton   *spinbutton,
                                gpointer         user_data)
{
  float sample_rate, time_slice;
  int samples;
  char *sample_label;


  sample_rate = process_get_sample_rate ();

  time_slice = gtk_spin_button_get_value (GTK_SPIN_BUTTON (user_data));

  process_set_rms_time_slice ((int) time_slice);

  samples = NINT ((time_slice / 1000.0) * sample_rate);

  sample_label = g_strdup_printf ("%d", samples);

  gtk_label_set_label (GTK_LABEL (spinbutton), sample_label);

  free (sample_label);
}

void
on_reset_hdeq_curve1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  hdeq_popup (0);
}


void
on_release_parametric_eq_controls1_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  hdeq_popup (1);
}


void
on_cancel2_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  hdeq_popup (2);
}


void
on_help2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  message (GTK_MESSAGE_INFO, _(hdeq_help));


  /*  Cancel any pre-existing hand-drawn curves.  */

  hdeq_popup (2);
}


void
on_scene_menu_help_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  message (GTK_MESSAGE_INFO, _(scenes_help));
}


void
on_pref_help_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
  message (GTK_MESSAGE_INFO, _(preferences_help));
}


void
on_limiter_combo_changed               (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
  /*  Important note - definition of limiters is in the same order as
      the combo box buttons.  Don't add to or rearrange the combo box
      entries unless you set the limiter definitions to match.  */

  process_set_limiter_plugin (gtk_combo_box_get_active (combobox));
}


void
on_SpectrumComboBox_changed            (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
  /*  Important note - definition of spectrum modes is in the same order as
      the combo box buttons.  Don't add to or rearrange the combo box
      entries unless you set the spectrum mode definitions to match.  */

  process_set_spec_mode(gtk_combo_box_get_active (combobox));
  on_EQ_curve_event_box_leave_notify_event (NULL, NULL, NULL);
}

void
on_ColorsComboBox_changed              (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
  /*  Important note - definition of colors is in the same order as
      the combo box buttons.  Don't add to or rearrange the combo box
      entries unless you set the color definitions to match.  */

  if (gtk_combo_box_get_active (combobox) == COLORS)
    {
      pref_reset_all_colors ();
      pref_force_color_change ();
    }
  else
    {
      popup_color_dialog (gtk_combo_box_get_active (combobox));
    }
}


void
on_LowDelayButton_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  process_set_xo_delay_state (XO_LOW, gtk_toggle_button_get_active (togglebutton));
}


void
on_MidDelayButton_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  process_set_xo_delay_state (XO_MID, gtk_toggle_button_get_active (togglebutton));
}


void
on_LowDelaySpinButton_value_changed    (GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
  process_set_xo_delay_time (XO_LOW, gtk_spin_button_get_value (spinbutton));
}


void
on_MidDelaySpinButton_value_changed    (GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
  process_set_xo_delay_time (XO_MID, gtk_spin_button_get_value (spinbutton));
}

void
on_logscale_scale_value_changed        (GtkRange        *range,
                                        gpointer         user_data)
{
  s_set_value_ui (S_LIM_LOGSCALE, gtk_adjustment_get_value(gtk_range_get_adjustment (GTK_RANGE (range))));
}

gboolean
on_pref_dialog_delete_event            (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  popup_pref_dialog (0);

  return TRUE;
}

gboolean
on_window2_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{

  return FALSE;
}


gboolean
on_window3_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    clean_quit ();
	
  return FALSE;
}



gboolean
on_eButton1_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	
	if(global_main_gui  == 1)
		global_main_gui = 0;
	else 
		global_main_gui = 1;
	
//	g_print("cllbk: clicked");
	presets_ui_show_main (widget);
	
  return FALSE;
}


gboolean
on_eButton2_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	
	if(global_multiout_gui  == 1)
		global_multiout_gui = 0;
	else 
		global_multiout_gui = 1;
	
//	g_print("cllbk: clicked");
	presets_ui_show_multiout (widget);
	
  return FALSE;
}

void
on_presets_in_trim_scale_value_changed (GtkRange        *range,
                                        gpointer         user_data)
{
	s_set_value_ui(S_IN_GAIN, gtk_adjustment_get_value(gtk_range_get_adjustment(range)));
	gtk_range_set_value (GTK_RANGE(lookup_widget(main_window, "in_trim_scale")), gtk_adjustment_get_value(gtk_range_get_adjustment(range)));

}


void
on_presets_pan_scale_value_changed     (GtkRange        *range,
                                        gpointer         user_data)
{
	s_set_value_ui(S_IN_PAN, gtk_adjustment_get_value(gtk_range_get_adjustment(range)));
	gtk_range_set_value (GTK_RANGE(lookup_widget(main_window, "pan_scale")), gtk_adjustment_get_value(gtk_range_get_adjustment(range)));
}


void
on_presets_out_trim_scale_value_changed
                                        (GtkRange        *range,
                                        gpointer         user_data)
{
	
    s_set_value_ui(S_OUT_GAIN, gtk_adjustment_get_value(gtk_range_get_adjustment(range)));
	gtk_range_set_value (GTK_RANGE(lookup_widget(main_window, "out_trim_scale")), gtk_adjustment_get_value(gtk_range_get_adjustment(range)));
}


void
on_in_trim_scale_value_changed         (GtkRange        *range,
                                        gpointer         user_data)
{
    s_set_value_ui(S_IN_GAIN, gtk_adjustment_get_value(gtk_range_get_adjustment(range)));
	gtk_range_set_value (GTK_RANGE(lookup_widget(presets_window, "presets_in_trim_scale")), gtk_adjustment_get_value(gtk_range_get_adjustment(range)));

}


void
on_pan_scale_value_changed             (GtkRange        *range,
                                        gpointer         user_data)
{
    s_set_value_ui(S_IN_PAN, gtk_adjustment_get_value(gtk_range_get_adjustment(range)));
	gtk_range_set_value (GTK_RANGE(lookup_widget(presets_window, "presets_pan_scale")), gtk_adjustment_get_value(gtk_range_get_adjustment(range)));

}

void
on_out_trim_scale_value_changed        (GtkRange        *range,
                                        gpointer         user_data)
{
    s_set_value_ui(S_OUT_GAIN, gtk_adjustment_get_value(gtk_range_get_adjustment(range)));
	gtk_range_set_value (GTK_RANGE(lookup_widget(presets_window, "presets_out_trim_scale")), gtk_adjustment_get_value(gtk_range_get_adjustment(range)));
}


gboolean
on_window4_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    clean_quit ();
	
  return FALSE;
}

