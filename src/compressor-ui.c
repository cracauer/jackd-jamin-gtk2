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
 *  $Id: compressor-ui.c,v 1.29 2008/02/04 15:03:50 esaracco Exp $
 */

#include <stdio.h>
#include <gtk/gtk.h>
#include <math.h>

#include "process.h"
#include "support.h"
#include "main.h"
#include "compressor-ui.h"
#include "gtkmeter.h"
#include "state.h"
#include "scenes.h"
#include "preferences.h"
#include "hdeq.h"


#define MUG_CORR_FACT 0.4f /* makeup gain correction factor - dampens the
			      makeup gain correction to stop it over
			      correcting */

gboolean adj_cb(GtkAdjustment *adj, gpointer p);
void at_changed(int id, float value);
void re_changed(int id, float value);
void th_changed(int id, float value);
void ra_changed(int id, float value);
void kn_changed(int id, float value);
void ma_changed(int id, float value);

void calc_auto_gain(int i);
void draw_comp_curve (int i, cairo_t * cr);

static GtkWidget *ma[XO_BANDS];
static GtkAdjustment *adj_at[XO_BANDS];
static GtkAdjustment *adj_re[XO_BANDS];
static GtkAdjustment *adj_th[XO_BANDS];
static GtkAdjustment *adj_ra[XO_BANDS];
static GtkAdjustment *adj_kn[XO_BANDS];
static GtkAdjustment *adj_ma[XO_BANDS];
static int auto_gain[XO_BANDS];

static GtkMeter *le_meter[XO_BANDS], *ga_meter[XO_BANDS];
static GtkAdjustment *le_meter_adj[XO_BANDS], *ga_meter_adj[XO_BANDS];


/*  Variables used for ganging the compressor controls.  */

static GtkLabel *lab_at[XO_BANDS];
static GtkLabel *lab_re[XO_BANDS];
static GtkLabel *lab_th[XO_BANDS];
static GtkLabel *lab_ra[XO_BANDS];
static GtkLabel *lab_kn[XO_BANDS];
static GtkLabel *lab_ma[XO_BANDS];

static GtkToggleButton *autobutton[XO_NBANDS];

static gboolean gang_at[XO_BANDS];
static gboolean gang_re[XO_BANDS];
static gboolean gang_th[XO_BANDS];
static gboolean gang_ra[XO_BANDS];
static gboolean gang_kn[XO_BANDS];
static gboolean gang_ma[XO_BANDS];
static gdouble range_at[2][XO_BANDS];
static gdouble range_re[2][XO_BANDS];
static gdouble range_th[2][XO_BANDS];
static gdouble range_ra[2][XO_BANDS];
static gdouble range_kn[2][XO_BANDS];
static gdouble range_ma[2][XO_BANDS];
static gdouble prev_value_at[XO_BANDS];
static gdouble prev_value_re[XO_BANDS];
static gdouble prev_value_th[XO_BANDS];
static gdouble prev_value_ra[XO_BANDS];
static gdouble prev_value_kn[XO_BANDS];
static gdouble prev_value_ma[XO_BANDS];
static gulong sig_hand_at[XO_BANDS];
static gulong sig_hand_re[XO_BANDS];
static gulong sig_hand_th[XO_BANDS];
static gulong sig_hand_ra[XO_BANDS];
static gulong sig_hand_kn[XO_BANDS];
static gulong sig_hand_ma[XO_BANDS];
static gboolean suspend_gang = FALSE;

#define connect_scale(sym, i, member, state_id) \
        gang_##sym[i] = FALSE; \
	snprintf(name, 255, "comp_" # sym "_label_%d", i+1); \
	lab_##sym[i] = GTK_LABEL(lookup_widget(main_window, name)); \
	snprintf(name, 255, "autobutton_%d", i+1); \
	autobutton[i] = GTK_TOGGLE_BUTTON(lookup_widget(main_window, name)); \
	snprintf(name, 255, "comp_" # sym "_%d", i+1); \
	scale = lookup_widget(main_window, name); \
	adj_##sym[i] = gtk_range_get_adjustment(GTK_RANGE(scale)); \
        range_##sym[0][i] = gtk_adjustment_get_lower(adj_##sym[i]); \
        range_##sym[1][i] = gtk_adjustment_get_upper(adj_##sym[i]); \
        prev_value_##sym[i] = gtk_adjustment_get_value(adj_##sym[i]); \
        s_set_callback(state_id, sym##_changed); \
	s_set_adjustment(state_id, adj_##sym[i]); \
	s_set_value(state_id, compressors[i].member, 0); \
	sig_hand_##sym[i] = g_signal_connect(G_OBJECT(adj_##sym[i]), "value-changed", G_CALLBACK(adj_cb), GINT_TO_POINTER (state_id)); 

//	g_signal_connect(G_OBJECT(adj_##sym[i]), "value-changed", G_CALLBACK(sym##_changed), (gpointer)i); 
//	gtk_adjustment_set_value(adj_##sym[i], compressors[i].member);

void bind_compressors()
{
    GtkWidget *scale;
    char name[256];
    int i;


    for (i=0; i<XO_BANDS; i++) {
	snprintf(name, 255, "comp_le_%d", i+1);
	le_meter[i] = GTK_METER(lookup_widget(main_window, name));
	le_meter_adj[i] = gtk_meter_get_adjustment(le_meter[i]);

	snprintf(name, 255, "comp_ga_%d", i+1);
	ga_meter[i] = GTK_METER(lookup_widget(main_window, name));
	ga_meter_adj[i] = gtk_meter_get_adjustment(ga_meter[i]);

	connect_scale(at, i, attack, S_COMP_ATTACK(i));
	connect_scale(re, i, release, S_COMP_RELEASE(i));
	connect_scale(th, i, threshold, S_COMP_THRESH(i));
	connect_scale(ra, i, ratio, S_COMP_RATIO(i));
	connect_scale(kn, i, knee, S_COMP_KNEE(i));
	connect_scale(ma, i, makeup_gain, S_COMP_MAKEUP(i));
	ma[i] = scale;

	auto_gain[i] = 0;
    }
}

gboolean adj_cb(GtkAdjustment *adj, gpointer p)
{
    s_set_value_ui(GPOINTER_TO_INT (p), gtk_adjustment_get_value(adj));

    return FALSE;
}

void at_changed(int id, float value)
{
  int          i, j;
  gdouble      diff, new_value;


  i = id - S_COMP_ATTACK(0);

  if (!suspend_gang && gang_at[i])
    {
      diff = value - prev_value_at[i];

      for (j = 0 ; j < XO_BANDS ; j++)
        {
          if (i != j && gang_at[j])
            {
              new_value = gtk_adjustment_get_value (adj_at[j]);
              new_value += diff;
              if (new_value >= range_at[0][j] && new_value <= range_at[1][j])
                {
                  g_signal_handler_block (adj_at[j], sig_hand_at[j]);

                  gtk_adjustment_set_value (adj_at[j], new_value);
                  compressors[j].attack = new_value;
                  prev_value_at[j] = new_value;

                  g_signal_handler_unblock (adj_at[j], sig_hand_at[j]);
                }
				comp_curve_update(j);
            }
        }
    }
                  
  prev_value_at[i] = value;


  compressors[i].attack = value;
  comp_curve_update(i);
}

void re_changed(int id, float value)
{
  int          i, j;
  gdouble      diff, new_value;


  i = id - S_COMP_RELEASE(0);

  if (!suspend_gang && gang_re[i])
    {
      diff = value - prev_value_re[i];

      for (j = 0 ; j < XO_BANDS ; j++)
        {
          if (i != j && gang_re[j])
            {
              new_value = gtk_adjustment_get_value (adj_re[j]);
              new_value += diff;
              if (new_value >= range_re[0][j] && new_value <= range_re[1][j])
                {
                  g_signal_handler_block (adj_re[j], sig_hand_re[j]);

                  gtk_adjustment_set_value (adj_re[j], new_value);
                  compressors[j].release = new_value;
                  prev_value_re[j] = new_value;

                  g_signal_handler_unblock (adj_re[j], sig_hand_re[j]);
                }
				comp_curve_update(j);
            }
        }
    }
                  
  prev_value_re[i] = value;


  compressors[i].release = value;
  comp_curve_update;
}

void th_changed(int id, float value)
{
  int          i, j;
  gdouble      diff, new_value;


  i = id - S_COMP_THRESH(0);

  if (!suspend_gang && gang_th[i])
    {
      diff = value - prev_value_th[i];

      for (j = 0 ; j < XO_BANDS ; j++)
        {
          if (i != j && gang_th[j])
            {
              new_value = gtk_adjustment_get_value (adj_th[j]);
              new_value += diff;
              if (new_value >= range_th[0][j] && new_value <= range_th[1][j])
                {
                  g_signal_handler_block (adj_th[j], sig_hand_th[j]);

                  gtk_adjustment_set_value (adj_th[j], new_value);
                  compressors[j].threshold = new_value;
                  prev_value_th[j] = new_value;

                  g_signal_handler_unblock (adj_th[j], sig_hand_th[j]);
                }
              if (auto_gain[j]) {
                calc_auto_gain(j);
              } else {
				comp_curve_update(j);
              }
              gtk_meter_set_warn_point(le_meter[j], new_value);
            }
        }
    }
                  
  prev_value_th[i] = value;


  compressors[i].threshold = value;
  if (auto_gain[i]) {
    calc_auto_gain(i);
  } else {
	comp_curve_update(i);
  }
  gtk_meter_set_warn_point(le_meter[i], value);
}

void ra_changed(int id, float value)
{
  int          i, j;
  gdouble      diff, new_value;


  i = id - S_COMP_RATIO(0);

  if (!suspend_gang && gang_ra[i])
    {
      diff = value - prev_value_ra[i];

      for (j = 0 ; j < XO_BANDS ; j++)
        {
          if (i != j && gang_ra[j])
            {
              new_value = gtk_adjustment_get_value (adj_ra[j]);
              new_value += diff;
              if (new_value >= range_ra[0][j] && new_value <= range_ra[1][j])
                {
                  g_signal_handler_block (adj_ra[j], sig_hand_ra[j]);

                  gtk_adjustment_set_value (adj_ra[j], new_value);
                  compressors[j].ratio = new_value;
                  prev_value_ra[j] = new_value;

                  g_signal_handler_unblock (adj_ra[j], sig_hand_ra[j]);
                }
              if (auto_gain[j]) {
                calc_auto_gain(j);
              } else {
				comp_curve_update(j);
              }
              gtk_meter_set_warn_point(le_meter[j], new_value);
            }
        }
    }
                  
  prev_value_ra[i] = value;


  compressors[i].ratio = value;
  if (auto_gain[i]) {
    calc_auto_gain(i);
  } else {
	comp_curve_update(i);
  }
  gtk_meter_set_warn_point(le_meter[i], value);
}

void kn_changed(int id, float value)
{
  int          i, j;
  gdouble      diff, new_value;


  i = id - S_COMP_KNEE(0);

  if (!suspend_gang && gang_kn[i])
    {
      diff = value - prev_value_kn[i];

      for (j = 0 ; j < XO_BANDS ; j++)
        {
          if (i != j && gang_kn[j])
            {
              new_value = gtk_adjustment_get_value (adj_kn[j]);
              new_value += diff;
              if (new_value >= range_kn[0][j] && new_value <= range_kn[1][j])
                {
                  g_signal_handler_block (adj_kn[j], sig_hand_kn[j]);

                  gtk_adjustment_set_value (adj_kn[j], new_value);
                  compressors[j].knee = new_value * 10.0f;
                  prev_value_kn[j] = new_value;

                  g_signal_handler_unblock (adj_kn[j], sig_hand_kn[j]);
                }
				comp_curve_update(j);
            }
        }
    }
                  
  prev_value_kn[i] = value;


  compressors[i].knee = value * 10.0f;
  comp_curve_update(i);
}

void ma_changed(int id, float value)
{
  int          i, j;
  gdouble      diff, new_value;
  char         *val;



  i = id - S_COMP_MAKEUP(0);

  if (!suspend_gang && gang_ma[i])
    {
      diff = value - prev_value_ma[i];

      for (j = 0 ; j < XO_BANDS ; j++)
        {
          if (i != j && gang_ma[j])
            {
              new_value = gtk_adjustment_get_value (adj_ma[j]);
              new_value += diff;
              if (new_value >= range_ma[0][j] && new_value <= range_ma[1][j])
                {
                  g_signal_handler_block (adj_ma[j], sig_hand_ma[j]);

                  gtk_adjustment_set_value (adj_ma[j], new_value);
                  compressors[j].makeup_gain = new_value;
                  prev_value_ma[j] = new_value;

                  g_signal_handler_unblock (adj_ma[j], sig_hand_ma[j]);

		  val = g_strdup_printf ("%04.1f", new_value);
		  gtk_button_set_label (GTK_BUTTON(autobutton[j]), val);
		  free (val);
                }
              comp_curve_update(j);
            }

        }
    }
                  
  prev_value_ma[i] = value;


  compressors[i].makeup_gain = value;

  val = g_strdup_printf ("%04.1f", value);
  gtk_button_set_label (GTK_BUTTON(autobutton[i]), val);
  free (val);

  comp_curve_update(i);
}

void calc_auto_gain(int i)
{
    if (adj_ma[i] && adj_th[i] && adj_ra[i]) {
	s_set_value_no_history(S_COMP_MAKEUP(i), (gtk_adjustment_get_value(adj_th[i]) / gtk_adjustment_get_value(adj_ra[i]) - gtk_adjustment_get_value(adj_th[i])) * MUG_CORR_FACT);
	gtk_adjustment_set_value(adj_ma[i], gtk_adjustment_get_value(adj_th[i]) / gtk_adjustment_get_value(adj_ra[i]) - gtk_adjustment_get_value(adj_th[i]));
    }
}

void compressor_meters_update()
{
    int i;

    for (i=0; i<XO_BANDS; i++) {
	 gtk_adjustment_set_value(le_meter_adj[i], compressors[i].amplitude);
	 gtk_adjustment_set_value(ga_meter_adj[i], compressors[i].gain_red);
    }
}

void comp_set_auto(int band, int state)
{
    auto_gain[band] = state;
    gtk_widget_set_sensitive(ma[band], !state);
    if (state) {
	calc_auto_gain(band);
    }

    set_scene_warning_button ();
}

comp_settings comp_get_settings(int band)
{
    return (compressors[band]);
}

void repaint_gang_labels ()
{
  int i;

  for (i = 0 ; i < XO_BANDS ; i++)
    {
      if (gang_at[i])
        {
          gtk_widget_modify_fg ((GtkWidget *) lab_at[i], GTK_STATE_NORMAL, 
                                get_color (GANG_HIGHLIGHT_COLOR));
        }
      else
        {
          gtk_widget_modify_fg ((GtkWidget *) lab_at[i], GTK_STATE_NORMAL, 
                                get_color (TEXT_COLOR));
        }
      if (gang_re[i])
        {
          gtk_widget_modify_fg ((GtkWidget *) lab_re[i], GTK_STATE_NORMAL, 
                                get_color (GANG_HIGHLIGHT_COLOR));
        }
      else
        {
          gtk_widget_modify_fg ((GtkWidget *) lab_re[i], GTK_STATE_NORMAL, 
                                get_color (TEXT_COLOR));
        }
      if (gang_th[i])
        {
          gtk_widget_modify_fg ((GtkWidget *) lab_th[i], GTK_STATE_NORMAL, 
                                get_color (GANG_HIGHLIGHT_COLOR));
        }
      else
        {
          gtk_widget_modify_fg ((GtkWidget *) lab_th[i], GTK_STATE_NORMAL, 
                                get_color (TEXT_COLOR));
        }
      if (gang_ra[i])
        {
          gtk_widget_modify_fg ((GtkWidget *) lab_ra[i], GTK_STATE_NORMAL, 
                                get_color (GANG_HIGHLIGHT_COLOR));
        }
      else
        {
          gtk_widget_modify_fg ((GtkWidget *) lab_ra[i], GTK_STATE_NORMAL, 
                                get_color (TEXT_COLOR));
        }
      if (gang_kn[i])
        {
          gtk_widget_modify_fg ((GtkWidget *) lab_kn[i], GTK_STATE_NORMAL, 
                                get_color (GANG_HIGHLIGHT_COLOR));
        }
      else
        {
          gtk_widget_modify_fg ((GtkWidget *) lab_kn[i], GTK_STATE_NORMAL, 
                                get_color (TEXT_COLOR));
        }
      if (gang_ma[i])
        {
          gtk_widget_modify_fg ((GtkWidget *) lab_ma[i], GTK_STATE_NORMAL, 
                                get_color (GANG_HIGHLIGHT_COLOR));
        }
      else
        {
          gtk_widget_modify_fg ((GtkWidget *) lab_ma[i], GTK_STATE_NORMAL, 
                                get_color (TEXT_COLOR));
        }
    }
}


void comp_gang_at (int band)
{
  if (gang_at[band])
    {
      gang_at[band] = FALSE;
      gtk_widget_modify_fg ((GtkWidget *) lab_at[band], GTK_STATE_NORMAL, 
                            get_color (TEXT_COLOR));
      prev_value_at[band] = gtk_adjustment_get_value (adj_at[band]);
    }
  else
    {
      gang_at[band] = TRUE;
      gtk_widget_modify_fg ((GtkWidget *) lab_at[band], GTK_STATE_NORMAL, 
                            get_color (GANG_HIGHLIGHT_COLOR));
    }
}

void comp_gang_re (int band)
{
  if (gang_re[band])
    {
      gang_re[band] = FALSE;
      gtk_widget_modify_fg ((GtkWidget *) lab_re[band], GTK_STATE_NORMAL, 
                            get_color (TEXT_COLOR));
      prev_value_re[band] = gtk_adjustment_get_value (adj_re[band]);
    }
  else
    {
      gang_re[band] = TRUE;
      gtk_widget_modify_fg ((GtkWidget *) lab_re[band], GTK_STATE_NORMAL, 
                            get_color (GANG_HIGHLIGHT_COLOR));
    }
}

void comp_gang_th (int band)
{
  if (gang_th[band])
    {
      gang_th[band] = FALSE;
      gtk_widget_modify_fg ((GtkWidget *) lab_th[band], GTK_STATE_NORMAL, 
                            get_color (TEXT_COLOR));
      prev_value_th[band] = gtk_adjustment_get_value (adj_th[band]);
    }
  else
    {
      gang_th[band] = TRUE;
      gtk_widget_modify_fg ((GtkWidget *) lab_th[band], GTK_STATE_NORMAL, 
                            get_color (GANG_HIGHLIGHT_COLOR));
    }
}

void comp_gang_ra (int band)
{
  if (gang_ra[band])
    {
      gang_ra[band] = FALSE;
      gtk_widget_modify_fg ((GtkWidget *) lab_ra[band], GTK_STATE_NORMAL, 
                            get_color (TEXT_COLOR));
      prev_value_ra[band] = gtk_adjustment_get_value (adj_ra[band]);
    }
  else
    {
      gang_ra[band] = TRUE;
      gtk_widget_modify_fg ((GtkWidget *) lab_ra[band], GTK_STATE_NORMAL, 
                            get_color (GANG_HIGHLIGHT_COLOR));
    }
}
void comp_gang_kn (int band)
{
  if (gang_kn[band])
    {
      gang_kn[band] = FALSE;
      gtk_widget_modify_fg ((GtkWidget *) lab_kn[band], GTK_STATE_NORMAL, 
                            get_color (TEXT_COLOR));
      prev_value_kn[band] = gtk_adjustment_get_value (adj_kn[band]);
    }
  else
    {
      gang_kn[band] = TRUE;
      gtk_widget_modify_fg ((GtkWidget *) lab_kn[band], GTK_STATE_NORMAL, 
                            get_color (GANG_HIGHLIGHT_COLOR));
    }
}

void comp_gang_ma (int band)
{
  if (gang_ma[band])
    {
      gang_ma[band] = FALSE;
      gtk_widget_modify_fg ((GtkWidget *) lab_ma[band], GTK_STATE_NORMAL, 
                            get_color (TEXT_COLOR));
      prev_value_ma[band] = gtk_adjustment_get_value (adj_ma[band]);
    }
  else
    {
      gang_ma[band] = TRUE;
      gtk_widget_modify_fg ((GtkWidget *) lab_ma[band], GTK_STATE_NORMAL, 
                            get_color (GANG_HIGHLIGHT_COLOR));
    }
}

void suspend_ganging ()
{
  suspend_gang = TRUE;
}

gboolean unsuspend_ganging (gpointer data)
{
  suspend_gang = FALSE;
  return (FALSE);
}

/* macro to create a trivial accesor function for the gang values */
#define GANG_ACCESOR(p) \
    gboolean comp_ ## p ## _ganged(int band) { return gang_ ## p[band]; }

GANG_ACCESOR(at);
GANG_ACCESOR(re);
GANG_ACCESOR(th);
GANG_ACCESOR(ra);
GANG_ACCESOR(kn);
GANG_ACCESOR(ma);

/* vi:set ts=8 sts=4 sw=4: */
