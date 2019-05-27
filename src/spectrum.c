/*
 *  Copyright (C) 2003 Jan C. Depner, Steve Harris
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
 *  $Id: spectrum.c,v 1.20 2006/07/10 23:05:32 jdepner Exp $
 */

#include <math.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include "support.h"
#include "main.h"
#include "process.h"
#include "gtkmeter.h"
//#include "gtkmeterscale.h"
#include "db.h"

static char *band_lbls[BANDS] = {
    "25.0", "31.5", "40.0", "50.0", "63.0", "80.0", "100", "125", "160", "200",
    "250",  "315",  "400",  "500",  "630",  "800",  "1k",  "1k25", "1k6", "2k",
    "2k5",  "3k1",  "4k",   "5k",   "6k3",  "8k",   "10k", "10k2", "16k", "20k"
};

GtkWidget *make_mini_label(const char *text);
static GtkAdjustment *adjustment[BANDS];

static int bin_bands[BINS];
static int band_bin[BANDS];
static gboolean timeout_ret = TRUE;
static int spectrum_freq = 10, timeout_countdown = 0;

void bind_spectrum()
{
    GtkWidget *root;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *meter;
    GtkWidget *mscale;
    int i, bin, band;
    float band_freq[BANDS];
    float band_bin_count[BANDS];

    root = lookup_widget(main_window, "spectrum_hbox");
    hbox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(root), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    vbox = gtk_vbox_new(FALSE, 1);
    gtk_widget_show(vbox);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
 //   mscale = gtk_meterscale_new(GTK_METERSCALE_RIGHT, LOWER_SPECTRUM_DB, 
 //                               UPPER_SPECTRUM_DB);
//    gtk_widget_show(mscale);
 //   gtk_box_pack_start(GTK_BOX(vbox), mscale, TRUE, TRUE, 0);
    label = make_mini_label(" ");
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);

    for (i = 0; i < BANDS; i++) {
		vbox = gtk_vbox_new(FALSE, 1);
		gtk_widget_show(vbox);
		gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

		adjustment[i] = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 
															  LOWER_SPECTRUM_DB, 
															  UPPER_SPECTRUM_DB,
															  0.0, 0.0, 0.0));
		meter = gtk_meter_new(adjustment[i], GTK_METER_UP,GTK_METERSCALE_TOP,LOWER_SPECTRUM_DB, UPPER_SPECTRUM_DB);
		gtk_meter_set_adjustment(GTK_METER(meter), adjustment[i]);
		//gtk_widget_set_usize(GTK_WIDGET(meter), 14, -1);
	//	gtk_meter_set_warn_point(GTK_METER(meter), 0.0);
		gtk_widget_show(meter);
		gtk_box_pack_start(GTK_BOX(vbox), meter, TRUE, TRUE, 0);

		label = make_mini_label(band_lbls[i]);
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
    }

    vbox = gtk_vbox_new(FALSE, 1);
    gtk_widget_show(vbox);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
 //   mscale = gtk_meterscale_new(GTK_METERSCALE_LEFT, LOWER_SPECTRUM_DB, 
 //                               UPPER_SPECTRUM_DB);
 //   gtk_widget_show(mscale);
 //   gtk_box_pack_start(GTK_BOX(vbox), mscale, TRUE, TRUE, 0);
    label = make_mini_label(" ");
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);

    /* Calcuate the centre frequency for each band */
    for (band=0; band<BANDS; band++) {
		band_freq[band] = 1000.0 * pow(10.0, (double)(band-16) * 0.1);
		//printf("band %d is at %f Hz\n", band, band_freq[band]);
		band_bin_count[band] = 0;
    }

    for (bin=0; bin<BINS/2; bin++) {
		const float bin_freq = (bin + 0.5f) * sample_rate / BINS;
		int nearest_band = 0;
		float nearest_dist = 9999999.0f;
		for (band=0; band<BANDS; band++) {
			if (fabs(bin_freq - band_freq[band]) < nearest_dist) {
			nearest_band = band;
			nearest_dist = fabs(bin_freq - band_freq[band]);
			}
		}
		bin_bands[bin] = nearest_band;
		//printf("bin %d (%f Hz) is nearest band %d (%f Hz)\n", bin, bin_freq, nearest_band, band_freq[nearest_band]);
		band_bin_count[nearest_band]++;
    }

    for (band=0; band<BANDS; band++) {
		if (band_bin_count[band] == 0) {
			band_bin[band] = band_freq[band] * BINS / sample_rate;
			//printf("band %d is unassigned, use bin %d\n", band, band_bin[band]);
		} else {
			/* Mark for no reverse lookup */
			band_bin[band] = -1;
		}
    }
}

gboolean spectrum_update(gpointer data)
{
    int i, page, count;
    float levels[BANDS];
    static float single_levels[BINS/2];

    void draw_EQ_spectrum_curve (float *);
    int get_current_notebook1_page ();

    float decay_rate = 0.2f;

    page = get_current_notebook1_page ();
    count = BINS / 2;

    if (page == 2) { // geq tab
      for (i=0; i<BANDS; i++) {
        levels[i] = 0.0f;
      }
      for (i=0; i<count; i++) {
        single_levels[i] = bin_peak_read_and_clear(i);
        levels[bin_bands[i]] += single_levels[i];
      }
      for (i=0; i<BANDS; i++) {
        if (band_bin[i] > 0) {
          levels[i] = (single_levels[band_bin[i]] +
                       single_levels[band_bin[i]+1]) * 0.5;
        }
//        printf("spectrum: setting adj %i \n", i);
        gtk_adjustment_set_value(adjustment[i], lin2db(levels[i]));
      }
    }
    else if (page == 0) { // hdeq tab
      for (i=0; i<BANDS; i++) {
        levels[i] = 0.0f;
      }
      for (i=0; i<count; i++) {
        single_levels[i] *= 1.0f - decay_rate;
        single_levels[i] += bin_peak_read_and_clear(i) * decay_rate;
      }
      draw_EQ_spectrum_curve (single_levels);
    }

    return (timeout_ret);
}


/*  This is a bit weird.  We want the timeout (above) to return FALSE so it
    will kill itself (if it's running).  We want to make sure that it's dead
    before we restart it with a new timeout.  What's happening here is that
    we're setting a flag that will start a countdown in spectrum_timeout_check.
    That function is called by update_meters (every 100 milliseconds).  We
    let it countdown for 1100 ms to make sure that spectrum_update has killed
    itself and then we start a new timeout at the new frequency.  Why are we
    doing it this way?  Because this sucker can get called way too frequently
    to try to set a timeout to turn it off and on again (it respawns).  It 
    would be much easier if there was a g_timeout_remove ;-)  JCD  */

void set_spectrum_freq (int freq)
{
  timeout_ret = FALSE;
  timeout_countdown = 11;
  spectrum_freq = freq;
}


int get_spectrum_freq ()
{
  return (spectrum_freq);
}


void spectrum_timeout_check()
{
  int milliseconds;

//	printf("spectrum: timeout check\n");
  if (spectrum_freq && timeout_countdown)
    {
      timeout_countdown--;

      if (!timeout_countdown) 
        {
            timeout_ret = TRUE;
            milliseconds = 1000 / spectrum_freq;
            g_timeout_add (milliseconds, spectrum_update, NULL);
//            printf("spectrum: a: timeout check\n");
        }
    }
}



GtkWidget *make_mini_label(const char *text)
{
    GtkLabel *label;
    char markup[256];

    label = GTK_LABEL(gtk_label_new(NULL));
    snprintf(markup, 255, "<span size=\"%d\">%s</span>", 8 * PANGO_SCALE, text);
    gtk_label_set_markup(label, markup);

    return GTK_WIDGET(label);
}

/* vi:set ts=8 sts=4 sw=4: */
