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
 *  $Id: geq.c,v 1.35 2013/02/05 01:34:01 kotau Exp $
 */

/* code to control the graphic eq's, swh */

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <math.h>

#include "geq.h"
#include "hdeq.h"
#include "process.h"
#include "support.h"
#include "main.h"
#include "db.h"
#include "state.h"
#include "scenes.h"
#include "callbacks.h"
#include "help.h"

GtkAdjustment *geqa[EQ_BANDS];
GtkRange *geqr[EQ_BANDS];

static int EQ_drawn = 0;
static char *errstr = NULL;

/* Linear gain of the 1/3rd octave EQ bands */
float geq_gains[EQ_BANDS + 1];
/* Frequency of each band of the EQ */
float geq_freqs[EQ_BANDS];
int bin_base[BINS];
float bin_delta[BINS];

gboolean eqb_changed(GtkAdjustment *adj, gpointer user_data);
void geq_set_gains();

void bind_geq()
{
    char name[16];
    int i, bin;
    float last_bin, next_bin;
    const double hz_per_bin = sample_rate / (double)BINS;

    for (i=0; i<EQ_BANDS; i++) {
	geq_freqs[i] = 1000.0 * pow(10.0, (double)(i-16) * 0.1);
	/* printf("GEQ band %d = %g Hz\n", i, geq_freqs[i]); */
    }

    for (i=0; i<EQ_BANDS; i++) {
		sprintf(name, "eqb%d", i+1);
		geqr[i] = GTK_RANGE(lookup_widget(main_window, name));
		geqa[i] = GTK_ADJUSTMENT(gtk_range_get_adjustment(GTK_RANGE(geqr[i])));
		g_signal_connect(G_OBJECT(geqa[i]), "value-changed", 
							 G_CALLBACK(eqb_changed), GINT_TO_POINTER (i+1));
		g_signal_connect(G_OBJECT(geqa[i]), "value-changed", 
							 G_CALLBACK(hdeq_eqb_mod), NULL);
	}


    for (i=0; i<BANDS + 1; i++) {
		geq_gains[i] = 1.0f;
    }

    bin = 0;
    while (bin <= geq_freqs[0] / hz_per_bin && bin < (BINS / 2) - 1) {
		bin_base[bin] = 0;
		bin_delta[bin++] = 0.0f;
    }

    for (i = 1; i < BANDS - 1 && bin < (BINS / 2) - 1
	 && geq_freqs[i+1] < sample_rate / 2; i++) {
		last_bin = bin;
		next_bin = geq_freqs[i+1] / hz_per_bin;
		while (bin <= next_bin) {
			bin_base[bin] = i;
			bin_delta[bin] = (float)(bin-last_bin) / (float)(next_bin-last_bin);
			bin++;
		}
    }

    for (; bin < (BINS / 2); bin++) {
		bin_base[bin] = BANDS - 1;
		bin_delta[bin] = 0.0f;
    }

    geq_set_gains();
}

void geq_set_gains()
{
    unsigned int bin;

    if (!EQ_drawn)
      {
//		printf("setting geq gains");  
        eq_coefs[0] = 1.0f;
        for (bin = 1; bin < (BINS/2 - 1); bin++) {
          eq_coefs[bin] = ((1.0f-bin_delta[bin]) * geq_gains[bin_base[bin]])
            + (bin_delta[bin] * geq_gains[bin_base[bin]+1]);
        }
      }
}

void geq_set_coefs (int length, float x[], float y[])
{
    int i, bin;


    if (length < BINS / 2 - 1)
      {
        errstr = 
          g_strdup_printf (_("Splined length %d does not match BINS / 2 - 1 (%d)"), 
                           length, BINS / 2 - 1);

        fprintf (stderr, "%s\n", errstr);
        message (GTK_MESSAGE_WARNING, errstr);
        free (errstr);
      }
    else
      {
        /*  Set eq_coefs using linear gain values.  */

        for (i = 0 ; i < (BINS/2 - 1) ; i++) 
          {
            /*  Figure out which eq_coeffs bin corresponds to this frequency.
                The FFT bins go from 0Hz to the input sample rate divided 
                by 2.  */

            bin = NINT (x[i] / sample_rate * ((float) BINS + 0.5f));
            eq_coefs[bin] = powf (10.0f, y[i]);
          }
      }
}


void geq_set_sliders(int length, float x[], float y[])
{
    int i, j;
    float value;


    if (length < BINS / 2 - 1)
      {
        errstr = g_strdup_printf (_("Splined length %d does not match BINS / 2 - 1 (%d)"), 
                                  length, BINS / 2 - 1);

        fprintf (stderr, "%s\n", errstr);
        message (GTK_MESSAGE_WARNING, errstr);
        free (errstr);
      }
    else
      {
        /*  Make sure that we don't reset the coefficients when we set the
            graphic EQ adjustments (see geq_set_gains).  */

        EQ_drawn = 1;


        /*  Set the faders in the graphic EQ.  First and last should be
            exact since that's what we splined to.  Use linear interpolation 
            for the others.  */

        gtk_adjustment_set_value (geqa[0], y[0] / 0.05);
        for (j = 1, i = 0 ; j < EQ_BANDS - 1 ; j++)
          {
            while (geq_freqs[j] > x[i]) i++;

            value = y[i - 1] + (y[i] - y[i - 1]) * ((geq_freqs[j] - x[i - 1]) /
                (x[i] - x[i - 1]));

            gtk_adjustment_set_value (geqa[j], value / 0.05);
          }
        gtk_adjustment_set_value (geqa[EQ_BANDS - 1], y[length - 1] / 0.05);


        /*  Release the restriction on the graphic EQ adjustments.  */

        EQ_drawn = 0;
      }
}

void geq_set_range(double min, double max)
{
    int             i;

    for (i = 0 ; i < EQ_BANDS ; i++)
      {
        gtk_range_set_range (geqr[i], min, max);
      }
}

void geq_get_freqs_and_gains(float *freqs, float *gains)
{
    int              i;

    for (i = 0 ; i < EQ_BANDS ; i++)
      {
        freqs[i] = geq_freqs[i];
        gains[i] = geq_gains[i];
      }
}
    
gboolean eqb_changed(GtkAdjustment *adj, gpointer user_data)
{
    int band = GPOINTER_TO_INT (user_data);

    geq_gains[band-1] = db2lin(gtk_adjustment_get_value(adj));

    geq_set_gains();


    /*  If the adjustment was made by hand set the scene warning.  If it was 
        set automatically by the set_EQ function we don't want to set it 
        because this could just be a scene change.  We are drawing the curve
        in order to set the state values.  */

    if (!EQ_drawn) 
      {
        set_scene_warning_button ();
  //      draw_EQ_curve ();
		hdeq_curve_update();
      }

    return FALSE;
}

GtkAdjustment *geq_get_adjustment(int band)
{
    if (band < 0 || band > EQ_BANDS) {
	fprintf(stderr, _("jam error: Adjustment from out-of-range band %d requested\n"), band);
	exit(1);
    }
    return geqa[band];
}

/* vi:set ts=8 sts=4 sw=4: */
