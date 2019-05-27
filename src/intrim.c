/*
 *  Copyright (C) 2003 Steve Harris
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
 *  $Id: intrim.c,v 1.19 2008/12/03 03:22:03 kotau Exp $
 */

#include <gtk/gtk.h>
#include <math.h>
#include <stdio.h>

#include "process.h"
#include "support.h"
#include "main.h"
#include "intrim.h"
#include "gtkmeter.h"
#include "state.h"
#include "db.h"

static GtkMeter *in_meter[4], *out_meter[4], *rms_meter[2];
static GtkAdjustment *in_meter_adj[4], *out_meter_adj[4], *rms_meter_adj[2];
static GtkLabel	*pan_label[2];
static GtkEntry *out_meter_text[2], *rms_meter_text[2];
static float inmeter_warn_level, outmeter_warn_level, rmsmeter_warn_level;
static gboolean out_meter_peak_pref = TRUE, rms_meter_peak_pref = TRUE;

void intrim_cb(int id, float value);
void outtrim_cb(int id, float value);
void inpan_cb(int id, float value);

float in_gain[2] = {1.0f, 1.0f};
float out_gain = 1.0f;
float in_trim_gain = 1.0f;
float in_pan_gain[2] = {1.0f, 1.0f};

void bind_intrim()
{
	
	in_meter[0] = GTK_METER(lookup_widget(main_window, "inmeter_l"));
    in_meter[1] = GTK_METER(lookup_widget(main_window, "inmeter_r"));
	in_meter[2] = GTK_METER(lookup_widget(presets_window, "presets_inmeter_l"));
    in_meter[3] = GTK_METER(lookup_widget(presets_window, "presets_inmeter_r"));
    in_meter_adj[0] = gtk_meter_get_adjustment(in_meter[0]);
    in_meter_adj[1] = gtk_meter_get_adjustment(in_meter[1]);
	in_meter_adj[2] = gtk_meter_get_adjustment(in_meter[2]);
    in_meter_adj[3] = gtk_meter_get_adjustment(in_meter[3]);
    gtk_adjustment_set_value(in_meter_adj[0], -60.0);
    gtk_adjustment_set_value(in_meter_adj[1], -60.0);
    gtk_adjustment_set_value(in_meter_adj[2], -60.0);
    gtk_adjustment_set_value(in_meter_adj[3], -60.0);

    out_meter[0] = GTK_METER(lookup_widget(main_window, "outmeter_l"));
    out_meter[1] = GTK_METER(lookup_widget(main_window, "outmeter_r"));
    out_meter[2] = GTK_METER(lookup_widget(presets_window, "presets_outmeter_l"));
    out_meter[3] = GTK_METER(lookup_widget(presets_window, "presets_outmeter_r"));	
    out_meter_adj[0] = gtk_meter_get_adjustment(out_meter[0]);
    out_meter_adj[1] = gtk_meter_get_adjustment(out_meter[1]);
	out_meter_adj[2] = gtk_meter_get_adjustment(out_meter[2]);
    out_meter_adj[3] = gtk_meter_get_adjustment(out_meter[3]);
    gtk_adjustment_set_value(out_meter_adj[0], -60.0);
    gtk_adjustment_set_value(out_meter_adj[1], -60.0);
    gtk_adjustment_set_value(out_meter_adj[2], -60.0);
    gtk_adjustment_set_value(out_meter_adj[3], -60.0);
    out_meter_text[0] = GTK_ENTRY (lookup_widget (main_window, "out_meter_text_l"));
    out_meter_text[1] = GTK_ENTRY (lookup_widget (main_window, "out_meter_text_r"));

    rms_meter[0] = GTK_METER(lookup_widget(main_window, "rmsmeter_l"));
    rms_meter[1] = GTK_METER(lookup_widget(main_window, "rmsmeter_r"));
    rms_meter_adj[0] = gtk_meter_get_adjustment(rms_meter[0]);
    rms_meter_adj[1] = gtk_meter_get_adjustment(rms_meter[1]);
    gtk_adjustment_set_value(rms_meter_adj[0], -60.0);
    gtk_adjustment_set_value(rms_meter_adj[1], -60.0);
    rms_meter_text[0] = GTK_ENTRY (lookup_widget (main_window, "rms_meter_text_l"));
    rms_meter_text[1] = GTK_ENTRY (lookup_widget (main_window, "rms_meter_text_r"));

    pan_label[0] = GTK_LABEL(lookup_widget(main_window, "pan_label"));
	pan_label[1] = GTK_LABEL(lookup_widget(presets_window, "presets_pan_label"));
    update_pan_label(0.0);


    s_set_callback(S_IN_GAIN, intrim_cb);
    s_set_adjustment(S_IN_GAIN, gtk_range_get_adjustment(GTK_RANGE(lookup_widget(main_window, "in_trim_scale"))));
 //   s_set_adjustment(S_IN_GAIN, gtk_range_get_adjustment(GTK_RANGE(lookup_widget(presets_window, "presets_in_trim_scale"))));

    s_set_callback(S_OUT_GAIN, outtrim_cb);
    s_set_adjustment(S_OUT_GAIN, gtk_range_get_adjustment(GTK_RANGE(lookup_widget(main_window, "out_trim_scale"))));
//    s_set_adjustment(S_OUT_GAIN, gtk_range_get_adjustment(GTK_RANGE(lookup_widget(presets_window, "presets_out_trim_scale"))));

    s_set_callback(S_IN_PAN, inpan_cb);
    s_set_adjustment(S_IN_PAN, gtk_range_get_adjustment(GTK_RANGE(lookup_widget(main_window, "pan_scale"))));
//	s_set_adjustment(S_IN_PAN, gtk_range_get_adjustment(GTK_RANGE(lookup_widget(presets_window, "presets_pan_scale"))));
	
}

void intrim_cb(int id, float value)
{
    in_trim_gain = db2lin(value);
    in_gain[0] = in_trim_gain * in_pan_gain[0];
    in_gain[1] = in_trim_gain * in_pan_gain[1];
}

void outtrim_cb(int id, float value)
{
    out_gain = db2lin(value);
}

void inpan_cb(int id, float value)
{
    in_pan_gain[0] = db2lin(value * -0.5f);
    in_pan_gain[1] = db2lin(value * 0.5f);
    in_gain[0] = in_trim_gain * in_pan_gain[0];
    in_gain[1] = in_trim_gain * in_pan_gain[1];
    update_pan_label(value);
}

void in_meter_value(float amp[])
{
    gtk_adjustment_set_value(in_meter_adj[0], lin2db(amp[0]));
    gtk_adjustment_set_value(in_meter_adj[1], lin2db(amp[1]));
	gtk_adjustment_set_value(in_meter_adj[2], lin2db(amp[0]));
    gtk_adjustment_set_value(in_meter_adj[3], lin2db(amp[1]));
    amp[0] = 0.0f;
    amp[1] = 0.0f;
}

void out_meter_value(float amp[])
{
    char tmp[256];
    float lamp[2];

    lamp[0] = lin2db (amp[0]);
    lamp[1] = lin2db (amp[1]);

    gtk_adjustment_set_value(out_meter_adj[0], lamp[0]);
    gtk_adjustment_set_value(out_meter_adj[1], lamp[1]);
	gtk_adjustment_set_value(out_meter_adj[2], lin2db(amp[0]));
    gtk_adjustment_set_value(out_meter_adj[3], lin2db(amp[1]));

    if (out_meter_peak_pref)
      {
        lamp[0] = gtk_meter_get_peak (out_meter[0]);
        lamp[1] = gtk_meter_get_peak (out_meter[1]);
      }
    else
      {
        if (lamp[0] < -60.0) lamp[0] = -60.0;
        if (lamp[1] < -60.0) lamp[1] = -60.0;
      }

    snprintf (tmp, 255, "%.1f", lamp[0]);
    gtk_entry_set_text (out_meter_text[0], tmp);
    snprintf (tmp, 255, "%.1f", lamp[1]);
    gtk_entry_set_text (out_meter_text[1], tmp);

    amp[0] = 0.0f;
    amp[1] = 0.0f;
}

void rms_meter_value(float amp[])
{
    char tmp[256];
    float lamp[2];

    lamp[0] = lin2db (amp[0]);
    lamp[1] = lin2db (amp[1]);

    gtk_adjustment_set_value(rms_meter_adj[0], lamp[0]);
    gtk_adjustment_set_value(rms_meter_adj[1], lamp[1]);

    if (rms_meter_peak_pref)
      {
        lamp[0] = gtk_meter_get_peak (rms_meter[0]);
        lamp[1] = gtk_meter_get_peak (rms_meter[1]);
      }
    else
      {
        if (lamp[0] < -60.0) lamp[0] = -60.0;
        if (lamp[1] < -60.0) lamp[1] = -60.0;
      }

    snprintf (tmp, 255, "%.1f", lamp[0]);
    gtk_entry_set_text (rms_meter_text[0], tmp);
    snprintf (tmp, 255, "%.1f", lamp[1]);
    gtk_entry_set_text (rms_meter_text[1], tmp);

    amp[0] = 0.0f;
    amp[1] = 0.0f;
}

void intrim_set_out_meter_peak_pref (gboolean pref)
{
  out_meter_peak_pref = pref;
}

gboolean intrim_get_out_meter_peak_pref ()
{
  return (out_meter_peak_pref);
}

void intrim_set_rms_meter_peak_pref (gboolean pref)
{
  rms_meter_peak_pref = pref;
}

gboolean intrim_get_rms_meter_peak_pref ()
{
  return (rms_meter_peak_pref);
}

void update_pan_label(float balance)
{
    char tmp[256];

    if (balance < -0.5f) {
      snprintf(tmp, 255, _("left %.0fdB"), -balance);
    } else if (balance > 0.5f) {
      snprintf(tmp, 255, _("right %.0fdB"), balance);
    } else {
      sprintf(tmp, _("centre"));
    }
    gtk_label_set_label(pan_label[0], tmp);
	gtk_label_set_label(pan_label[1], tmp);
}

void intrim_inmeter_reset_peak ()
{
  gtk_meter_reset_peak (in_meter[0]);
  gtk_meter_reset_peak (in_meter[1]);
}

void intrim_outmeter_reset_peak ()
{
  gtk_meter_reset_peak (out_meter[0]);
  gtk_meter_reset_peak (out_meter[1]);
}

void intrim_rmsmeter_reset_peak ()
{
  gtk_meter_reset_peak (rms_meter[0]);
  gtk_meter_reset_peak (rms_meter[1]);
}

void intrim_inmeter_set_warn (float level)
{
  inmeter_warn_level = level;

  gtk_meter_set_warn_point (in_meter[0], level);
  gtk_meter_set_warn_point (in_meter[1], level);
}

void intrim_outmeter_set_warn (float level)
{
  outmeter_warn_level = level;
  gtk_meter_set_warn_point (out_meter[0], level);
  gtk_meter_set_warn_point (out_meter[1], level);
}

void intrim_rmsmeter_set_warn (float level)
{
  rmsmeter_warn_level = level;
  gtk_meter_set_warn_point (rms_meter[0], level);
  gtk_meter_set_warn_point (rms_meter[1], level);
}

float intrim_inmeter_get_warn ()
{
  return (inmeter_warn_level);
}

float intrim_outmeter_get_warn ()
{
  return (outmeter_warn_level);
}

float intrim_rmsmeter_get_warn ()
{
  return (rmsmeter_warn_level);
}


/* vi:set ts=8 sts=4 sw=4: */
