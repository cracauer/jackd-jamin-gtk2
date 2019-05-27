/*
 *  hdeq.c -- Hand drawn EQ, crossover, and compressor graph interface for
 *            the JAMin (JACK Audio Mastering interface) program.
 *
 *  Copyright (C) 2003 Jan C. Depner.
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*

    A few programmer notes:

    We use the GEQ to set up a lot of the HDEQ.  This is because the GEQ was 
    built first and we already had a few things, like the X and Y ranges, to 
    work from.  If you modify the GEQ the HDEQ will be reset.  If you modify
    the HDEQ the GEQ will be reset.  The difference between setting the EQ 
    curve with the HDEQ vs the GEQ is that you can actually set the 1024 bands
    of EQ instead of setting 31 bands and then splining the in-between points.

    The HDEQ uses a log scale in the X, or frequency, direction which is normal
    for audio.

    The X range of the HDEQ is set 25Hz to 20KHz.  This doesn't change, 
    however, the index into the X array of single_levels[] values (from 
    process.c) changes with the sample rate.  This is very confusing (I still 
    don't have it completely under control).

    A lot of the information that is used in process.c and state.c is not
    stored in frequency and gain (dB).  It is in other units - you'll have to
    look at the code 'cause I don't remember exactly what they are at the 
    moment ;-)

    Functions that begin with process_ are defined in the process.c file.  
    Functions that begin with s_ are defined in state.c.  We've tried to
    stay with this as much as possible (although it's not written in stone).
    You'll note that a lot of the functions that are callable outside this
    function begin with hdeq_.

    We're trying to stay away from extern'ed global variables as much as
    possible (I've been tainted by C++  ;-)  If you need to access a variable
    that is used here (set or get) write a little one line function that
    returns or sets it.  You can call it hdeq_set_... or hdeq_get_...  Yes,
    there is some overhead associated with it but it makes tracking things
    much easier.

    Are there a lot of comments in this code?  Yes.  If I don't do this the
    Alzheimers kicks my butt when I come back to work on it.

    Oh, one last thing.  Steve's a Brit so if you see things like "colour",
    "centre", "defence", or "anorak" just go with the flow :D

*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
//#include <cairo.h>

#include "hdeq.h"
#include "main.h"
#include "callbacks.h"
#include "geq.h"
#include "interface.h"
#include "support.h"
#include "intrim.h"
#include "compressor-ui.h"
//#include "gtkmeter.h"
//#include "gtkmeterscale.h"
#include "state.h"
#include "db.h"
#include "transport.h"
#include "scenes.h"
#include "spectrum.h"
#include "preferences.h"


/*  What I'm referring to as notches are actually slidable parametric EQ 
    controls.  */

#define EQ_SPECTRUM_RANGE             90.0
#define XOVER_HANDLE_SIZE             10
#define XOVER_HANDLE_HALF_SIZE        (XOVER_HANDLE_SIZE / 2)
#define NOTCH_CENTER_HEIGHT           32
#define NOTCH_CENTER_HALF_HEIGHT      (NOTCH_CENTER_HEIGHT / 2)
#define NOTCH_HANDLE_HEIGHT           16
#define NOTCH_HANDLE_HALF_HEIGHT      (NOTCH_HANDLE_HEIGHT / 2)
#define NOTCH_HANDLE_WIDTH            8
#define NOTCH_HANDLE_HALF_WIDTH       (NOTCH_HANDLE_WIDTH / 2)
#define MOTION_CLOCK_DIFF             ((int) (sysconf (_SC_CLK_TCK) * 0.05))


/*  Number of points used for the falloff in high/low pass notches.  You can
    make the slope shallower by increasing this or steeper by decreasing.  */

#define NOTCH_PASS_WIDTH              75


void interpolate (float, int, float, float, int *, float *, float *, float *, 
                  float *);


/* vi:set ts=8 sts=4 sw=4: */


/*  HDEQ right-click popup menu.  */

static GtkMenu           *HDEQ_menu;


/*  This is the offscreen drawable on which we paint our "static"
    background elements (gain lines, crossover bars, EQ curve, notch
    handles, etc).  Anytime these elements change or we paint the
    spectrum curve we blast this pixmap back to the widget (EQ_drawable).
    We paint the spectrum curve directly on the widget.  This way we can
    avoid using XOR graphics and all of the pain and agony that entails
    ;-)  */

//static GdkPixmap *hdeq_pixmap = NULL;


static GtkHScale       *l_low2mid, *l_mid2high;
static GtkWidget       *l_comp[3];
static GtkLabel        *l_low2mid_lbl, *l_mid2high_lbl, *l_comp_lbl[3], 
                       *l_EQ_curve_lbl = NULL, *l_c_curve_lbl[3];
static GtkDrawingArea  *l_EQ_curve, *l_comp_curve[3];
//static GdkDrawable     *EQ_drawable, *comp_drawable[3];
//static GdkGC           *EQ_gc, *comp_gc[3];
static cairo_t 		   *EQ_cr, *comp_cr[3];
static cairo_surface_t *EQ_surface, *comp_surface[3];
static PangoContext    *comp_pc[3], *EQ_pc;
static GtkAdjustment   *l_low2mid_adj;
static GdkColor 	   *color;
static float           EQ_curve_range_x, EQ_curve_range_y, EQ_curve_width,
                       EQ_curve_height, EQ_xinterp[EQ_INTERP + 1], EQ_start, 
                       EQ_end, EQ_interval, EQ_yinterp[EQ_INTERP + 1], 
                       *EQ_xinput = NULL, *EQ_yinput = NULL, 
                       l_geq_freqs[EQ_BANDS], l_geq_gains[EQ_BANDS], 
                       comp_curve_range_x[3], comp_curve_range_y[3], 
                       comp_curve_width[3], comp_curve_height[3] , 
                       comp_start_x[3], comp_start_y[3], comp_end_x[3], 
                       comp_end_y[3], EQ_freq_xinterp[EQ_INTERP + 1],
                       EQ_freq_yinterp[EQ_INTERP + 1], 
                       EQ_notch_gain[NOTCHES], EQ_x_notched[EQ_INTERP + 1], 
                       EQ_y_notched[EQ_INTERP + 1], EQ_gain_lower = -12.0, 
                       EQ_gain_upper = 12.0, EQ_notch_default[NOTCHES];

/*  Note the array dimensions for the parametric notch handles.  Here's how 
    they're laid out - first dimension [2] = X and Y indexes; second dimension [3] =
    left handle, center handle, right handle (left shelf 0 is 0, right shelf 2 is
    EQ_CURVE_width); third dimension is notch number from left (0).  */

static int             EQ_mod = 1, EQ_drawing = 0, EQ_input_points = 0, 
                       EQ_length = 0, EQ_draw_dir = 0, 
                       comp_realized[3] = {0, 0, 0}, 
                       EQ_realized = 0, xover_active = 0, xover_handle_l2m, 
                       xover_handle_m2h, EQ_drag_l2m = 0, EQ_drag_m2h = 0, 
                       EQ_notch_drag[NOTCHES],
                       EQ_notch_Q_drag[NOTCHES], 
                       EQ_notch_handle[2][3][NOTCHES], EQ_notch_width[NOTCHES],
                       EQ_notch_index[NOTCHES], EQ_notch_flag[NOTCHES];
static guint           notebook1_page = 0;
static gboolean        hdeq_ready = FALSE;


/*  Saving the last drawn spectrum curve so we can keep it visible when moving
    anything in the hdeq window (see draw_EQ_spectrum_curve).  */

static int             spectrum_x[EQ_INTERP], spectrum_y[EQ_INTERP];



/*  Given the frequency this returns the nearest array index in the X direction
    in the hand drawn EQ curve.  This will be one of 1024 values.  */

static int nearest_x (float freq)
{
    int          i, j = 0;
    float        dist, ndist;


    dist = 99999999.0;

    for (i = 0 ; i < EQ_length ; i++)
      {
        ndist = log10f (freq);
        if (fabs (ndist - EQ_xinterp[i]) < dist)
          {
            dist = fabs (ndist - EQ_xinterp[i]);
            j = i;
          }
      }

  return (j);
}


/*  Clear out the hand drawn EQ curves on exit.  */

void clean_quit ()
{
    if (EQ_xinput) free (EQ_xinput);
    if (EQ_yinput) free (EQ_yinput);


    /*  Write out the defaults file in case we've changed something.  */

    pref_write_jamin_defaults ();

    /* free global session filename */

    s_set_session_filename (NULL);

    gtk_main_quit();
}


/*  Setup default values and widget pointers based on names from glade-2.  
    DON'T CHANGE WIDGET NAMES in glade-2 without checking first.  */

void bind_hdeq ()
{
    int    i;


    /*  The HDEQ right click popup menu (see interface.c for create_HDEQ_menu). */

    HDEQ_menu = (GtkMenu *) create_HDEQ_menu();


    /*  Looking up the widgets we'll need to work with based on the name
        that was set in glade-2.  If you change the widget name in glade-2
        you'll break the app.  */

    l_low2mid = GTK_HSCALE (lookup_widget (main_window, "low2mid"));
    l_mid2high = GTK_HSCALE (lookup_widget (main_window, "mid2high"));
    l_low2mid_lbl = GTK_LABEL (lookup_widget (main_window, "low2mid_lbl"));
    l_mid2high_lbl = GTK_LABEL (lookup_widget (main_window, "mid2high_lbl"));

    l_comp[0] = lookup_widget (main_window, "frame_l");
    l_comp[1] = lookup_widget (main_window, "frame_m");
    l_comp[2] = lookup_widget (main_window, "frame_h");

    l_comp_lbl[0] = GTK_LABEL (lookup_widget (main_window, "label_freq_l"));
    l_comp_lbl[1] = GTK_LABEL (lookup_widget (main_window, "label_freq_m"));
    l_comp_lbl[2] = GTK_LABEL (lookup_widget (main_window, "label_freq_h"));

    l_EQ_curve = GTK_DRAWING_AREA (lookup_widget (main_window, "EQ_curve"));
    l_EQ_curve_lbl = GTK_LABEL (lookup_widget (main_window, "EQ_curve_lbl"));

    l_comp_curve[0] = GTK_DRAWING_AREA (lookup_widget (main_window, 
                                                       "comp1_curve"));
    l_comp_curve[1] = GTK_DRAWING_AREA (lookup_widget (main_window, 
                                                       "comp2_curve"));
    l_comp_curve[2] = GTK_DRAWING_AREA (lookup_widget (main_window, 
                                                       "comp3_curve"));

    l_c_curve_lbl[0] = GTK_LABEL (lookup_widget (main_window, 
                                                 "low_curve_lbl"));
    l_c_curve_lbl[1] = GTK_LABEL (lookup_widget (main_window, 
                                                 "mid_curve_lbl"));
    l_c_curve_lbl[2] = GTK_LABEL (lookup_widget (main_window, 
                                                 "high_curve_lbl"));


    /*  All of the notch defaults.  Note that EQ_notch_index is NOT the 
        frequency of the notch handle.  It is the index into the frequncy
        array.  Also, the X scale for the HDEQ is log.  This is pretty
        much standard for audio.  */

    EQ_notch_default[0] = 29.0;
    EQ_notch_default[1] = 131.0;
    EQ_notch_default[2] = 710.0;
    EQ_notch_default[3] = 3719.0;
    EQ_notch_default[4] = 16903.0;

    for (i = 0 ; i < NOTCHES ; i++)
      {
        EQ_notch_gain[i] = 0.0;
        EQ_notch_width[i] = 5;
        EQ_notch_drag[i] = 0; 
        EQ_notch_Q_drag[i] = 0;
        EQ_notch_flag[i] = 0;
        EQ_notch_index[i] = nearest_x (EQ_notch_default[i]);
      }

    EQ_notch_width[0] = 0;
    EQ_notch_width[NOTCHES - 1] = 0;

}


/*  This is here so that other functions can reset to these if needed.  */

float hdeq_get_notch_default_freq (int i)
{
  return (EQ_notch_default[i]);
}


/*  Setting the low to mid crossover.  Called from callbacks.c.  */

void hdeq_low2mid_set (GtkRange *range)
{
    double          value, other_value, lvalue, mvalue, hvalue;
    char            *label = NULL;
    int             i;


    /*  Get the value from the crossover range widget.  */

    value = gtk_range_get_value (range);
    other_value = gtk_range_get_value ((GtkRange *) l_mid2high);
    s_set_value_ui(S_XOVER_FREQ(0), value);


    /*  Don't let the two sliders cross each other and desensitize the mid
        band compressor if they are the same value.  */

    if (value >= other_value)
      {
        /*  This tells the state functions (state.c) not to do anything even 
            though we're going to move a GUI control.  */

		s_suppress_push();

        gtk_range_set_value ((GtkRange *) l_mid2high, value);


        /*  This lets the state functions behave normally again.  */

		s_suppress_pop();
        gtk_widget_set_sensitive (l_comp[1], FALSE);
      }
    else
      {
        gtk_widget_set_sensitive (l_comp[1], TRUE);
      }


    /*  If the low slider is at the bottom of it's range, desensitize the low 
        band compressor.  */

    l_low2mid_adj = gtk_range_get_adjustment (range);

    if (value == gtk_adjustment_get_lower (l_low2mid_adj))
      {
        gtk_widget_set_sensitive (l_comp[0], FALSE);
      }
    else
      {
        gtk_widget_set_sensitive (l_comp[0], TRUE);
      }


    /*  Set the label using log scale.  */

    lvalue = pow (10.0, value);
    label = g_strdup_printf("%05d", NINT (lvalue));
    gtk_label_set_label (l_low2mid_lbl, label);
    free(label);


    /*  Here we're setting the frequency of the low to mid band crossover.  */

    process_set_low2mid_xover (lvalue);


    /*  Set the compressor labels.  */

    hvalue = pow (10.0, other_value);
    label = g_strdup_printf (_("<b>Mid : %d - %d</b>"), NINT (lvalue), NINT (hvalue));
    gtk_label_set_label (l_comp_lbl[1], label);
    gtk_label_set_use_markup (l_comp_lbl[1], TRUE);
    free(label);

    lvalue = pow (10.0, gtk_adjustment_get_lower (l_low2mid_adj));
    mvalue = pow (10.0, value);
    label = g_strdup_printf(_("<b>Low : %d - %d</b>"), NINT (lvalue), NINT (mvalue));
    gtk_label_set_label (l_comp_lbl[0], label);
    gtk_label_set_use_markup (l_comp_lbl[0], TRUE);
    free(label);


    /*  Replot the EQ curve (and set a few things at the same time).  */

//    draw_EQ_curve ();


    /*  Draw the last spectrum curve if the update frequency is not 0.  */

    if (get_spectrum_freq ())
      {
        /*  Set the foreground color for drawing the spectrum curve.  */

       // gdk_gc_set_foreground (EQ_gc, get_color (HDEQ_SPECTRUM_COLOR));
		color = get_color (HDEQ_SPECTRUM_COLOR);
	//	cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);
		
	// cr = gdk_cairo_create (widget->window);	
	//cairo_rectangle (cr, 0, length, width-1, 2);
	//cairo_set_source_rgba (cr, 0.5,0.5,0.5, 1.0); 
	//cairo_fill (cr);
	//  cairo_destroy(cr);  
		
		
        for (i = 0 ; i < EQ_INTERP ; i++)
          {
        //    if (i) gdk_draw_line (EQ_drawable, EQ_gc, spectrum_x[i - 1], spectrum_y[i - 1],
        //                          spectrum_x[i], spectrum_y[i]);
          }


            /*  Reset the foreground color to the text color.  */

       // gdk_gc_set_foreground (EQ_gc, get_color (TEXT_COLOR));
      }
}


/*  Setting the mid to high crossover.  Called from callbacks.c.  */

void hdeq_mid2high_set (GtkRange *range)
{
    double          value, other_value, lvalue, mvalue, hvalue;
    char            *label = NULL;
    int             i;


    /*  Get the value from the crossover range widget.  */

    value = gtk_range_get_value (range);
    other_value = gtk_range_get_value ((GtkRange *) l_low2mid);
    s_set_value_ui(S_XOVER_FREQ(1), value);


    /*  Don't let the two sliders cross each other and desensitize the mid
        band compressor if they are the same value.  */

    if (value <= other_value)
      {
        /*  Suppress state functionality.  */

	s_suppress_push();

        gtk_range_set_value ((GtkRange *) l_low2mid, value);


        /*  Unsuppress state functionality.  */

	s_suppress_pop();
        gtk_widget_set_sensitive (l_comp[1], FALSE);
      }
    else
      {
        gtk_widget_set_sensitive (l_comp[1], TRUE);
      }


    /*  If the slider is at the top of it's range, desensitize the high band
        compressor.  */


    if (value == gtk_adjustment_get_upper (l_low2mid_adj))
      {
        gtk_widget_set_sensitive (l_comp[2], FALSE);
      }
    else
      {
        gtk_widget_set_sensitive (l_comp[2], TRUE);
      }


    /*  Set the label using log scale.  */

    mvalue = pow (10.0, value);
    label = g_strdup_printf ("%05d", NINT (mvalue));
    gtk_label_set_label (l_mid2high_lbl, label);
    free(label);


    /*  Set the frequency of the mid to high band crossover.  */

    process_set_mid2high_xover (mvalue);


    /*  Set the compressor labels.  */

    lvalue = pow (10.0, other_value);
    label = g_strdup_printf (_("<b>Mid : %d - %d</b>"), NINT (lvalue), NINT (mvalue));
    gtk_label_set_label (l_comp_lbl[1], label);
    gtk_label_set_use_markup (l_comp_lbl[1], TRUE);
    free(label);

    hvalue = pow (10.0, gtk_adjustment_get_upper (l_low2mid_adj));
    label = g_strdup_printf (_("<b>High : %d - %d</b>"), NINT (mvalue), NINT (hvalue));
    gtk_label_set_label (l_comp_lbl[2], label);
    gtk_label_set_use_markup (l_comp_lbl[2], TRUE);
    free(label);


    /*  Replot the EQ curve (and set a few things at the same time).  */

 //   draw_EQ_curve ();


    /*  Draw the last spectrum curve if the update frequency is not 0.  */

    if (get_spectrum_freq ())
      {
        /*  Set the foreground color for drawing the spectrum curve.  */

 //       gdk_gc_set_foreground (EQ_gc, get_color (HDEQ_SPECTRUM_COLOR));
		color = get_color (HDEQ_SPECTRUM_COLOR);
//		cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);
        for (i = 0 ; i < EQ_INTERP ; i++)
          {
       //     if (i) gdk_draw_line (EQ_drawable, EQ_gc, spectrum_x[i - 1], spectrum_y[i - 1],
       //                           spectrum_x[i], spectrum_y[i]);
          }


            /*  Reset the foreground color to the text color.  */

     //   gdk_gc_set_foreground (EQ_gc, get_color (TEXT_COLOR));
      }
}


/*  Someone has pressed the low to mid crossover button.  Called from 
    callbacks.c.  */

void hdeq_low2mid_button (int active)
{
    /*  Set the active flag.  */

    xover_active = active;
}


/*  Someone has pressed the mid to high crossover button.  Called from 
    callbacks.c.  */

void hdeq_mid2high_button (int active)
{
    /*  Set the active flag.  */

    xover_active = active;
}


/*  Initialize the low to mid crossover adjustment state.  */

void hdeq_low2mid_init ()
{
    s_set_adjustment (S_XOVER_FREQ(0), gtk_range_get_adjustment(GTK_RANGE(l_low2mid)));
}


/*  Initialize the mid to high crossover adjustment state.  */

void hdeq_mid2high_init ()
{
    s_set_adjustment (S_XOVER_FREQ(1), gtk_range_get_adjustment(GTK_RANGE(l_mid2high)));
}


/*  Set the low to mid and mid to high crossovers.  This is called from the 
    window1_show callback (in callbacks.c) so it only gets called once.  */

void crossover_init ()
{
    hdeq_low2mid_set ((GtkRange *) l_low2mid);
    hdeq_mid2high_set ((GtkRange *) l_mid2high);
}


/*  If we've modified the graphic EQ (geq) then we want to build the hand
    drawn EQ from the geq.  EQ_mod flag will cause that to happen on the next
    redraw (draw_EQ_curve).  This is a callback that is set up in the GEQ
    code.  */

gboolean 
hdeq_eqb_mod (GtkAdjustment *adj, gpointer user_data)
{
    EQ_mod = 1;

    return FALSE;
}


/*  Convert log frequency to X pixels in the hdeq.  */

static void logfreq2xpix (float log_freq, int *x)
{
  *x = NINT (((log_freq - gtk_adjustment_get_lower (l_low2mid_adj)) / EQ_curve_range_x) * 
        EQ_curve_width);
        
  //      printf("%f, %f, %i, %i\n",log_freq, gtk_adjustment_get_lower (l_low2mid_adj),  EQ_curve_range_x, 
  //      EQ_curve_width );
}


/*  Convert frequency to X pixels in the hdeq.  */

static void freq2xpix (float freq, int *x)
{
    float log_freq;


    /*  Covert the frequency to log of the frequency and call the above 
        function.  */

    log_freq = log10f (freq);
    logfreq2xpix (log_freq, x);
}


/*  Convert gain to Y pixels in the hdeq.  */

static void gain2ypix (float gain, int *y)
{
    *y = EQ_curve_height - NINT (((gain - EQ_gain_lower) / 
                EQ_curve_range_y) * EQ_curve_height);
}


/*  Convert log gain to Y pixels in the hdeq.  */

static void loggain2ypix (float log_gain, int *y)
{
    float gain;


    /*  Convert the gain to log of the gain and call the above functio.  */

    gain = log_gain * 20.0;
    gain2ypix (gain, y);
}


/*  Draw the spectrum in the hdeq window.  This is called from spectrum_update
    (in main.c) which is called based on the timer set up in main.c.  */

void draw_EQ_spectrum_curve (float single_levels[])
{
    int            i, bin;
    float          step, range, freq;	

    /*  Don't update if we're drawing an EQ curve or we're moving a 
        crossover.  */

    if (!EQ_drawing && !xover_active)
      {


        /*  Convert the single levels to db, plot, and save the pixel positions
            so that we can erase them on the next pass.  Note that we're
            setting our range based on the GEQ range.  */

        range = l_geq_freqs[EQ_BANDS - 1] - l_geq_freqs[0];
        step = range / (float) EQ_INTERP;
        for (i = 0 ; i < EQ_INTERP ; i++)
          {
            freq = l_geq_freqs[0] + (float) i * step;


            freq2xpix (freq, &spectrum_x[i]);


            /*  Figure out which single_levels[] bin corresponds to this 
                frequency.  The FFT bins go from 0Hz to the input sample rate
                divided by 2.  */

            bin = NINT (freq / sample_rate * ((float) BINS + 0.5f));


            /*  Most of the single_levels[] values will be in the -90.0db to 
                -20.0db range.  We're using -90.0db to 0.0db as our range.  */

            spectrum_y[i] = NINT (-(lin2db(single_levels[bin]) / EQ_SPECTRUM_RANGE) * 
                                  EQ_curve_height);
        
			
          }

		 gtk_widget_queue_draw(GTK_WIDGET(l_EQ_curve)); 

      }
        
}


/*  Set the graphic EQ (geq) sliders and the full set of EQ coefficients
    based on the hand drawn EQ curve.  */

static void set_EQ ()
{
    float    *x = NULL, interval;
    int      i, size;


    /*  Make sure we have enough space.  */

    size = EQ_length * sizeof (float);
    x = (float *) realloc (x, size);

    if (x == NULL)
      {
        perror (_("Allocating x in set_EQ"));
        clean_quit ();
      }


    /*  Recompute the splined curve in the freq domain for setting the 
        eq_coefs.  */

    for (i = 0 ; i < EQ_length ; i++) x[i] = powf (10.0f, EQ_x_notched[i]);

    interval = ((l_geq_freqs[EQ_BANDS - 1]) - l_geq_freqs[0]) / EQ_INTERP;

    interpolate (interval, EQ_length, l_geq_freqs[0], 
        l_geq_freqs[EQ_BANDS - 1], &EQ_length, x, EQ_y_notched, 
        EQ_freq_xinterp, EQ_freq_yinterp);


    if (x) free (x);


    /*  Set EQ coefficients based on the hand-drawn curve.  */
    	geq_set_coefs (EQ_length, EQ_freq_xinterp, EQ_freq_yinterp);

    /*  Set the graphic EQ sliders based on the hand-drawn curve.  */
    	geq_set_sliders (EQ_length, EQ_freq_xinterp, EQ_freq_yinterp);
	
	//printf("%f, %f, %f\n",EQ_length, EQ_freq_xinterp, EQ_freq_yinterp);
		
    EQ_mod = 0;
}


/*  Reset the curve and parametric controls to 0.  */

void reset_hdeq ()
{
    int                  i;


    /*  Setting the EQ (and state).  */

    for (i = 0 ; i < EQ_length ; i++) EQ_y_notched[i] = EQ_yinterp[i] = 0.0;
    s_set_value_block (EQ_yinterp, S_EQ_GAIN(0), EQ_length);


    /*  Setting the notches (and state).  */

    for (i = 0 ; i < NOTCHES ; i++)
      {
        EQ_notch_gain[i] = 0.0;
        EQ_notch_drag[i] = 0;
        EQ_notch_Q_drag[i] = 0;
        EQ_notch_flag[i] = 0;
        if (!i || i == NOTCHES - 1)
          {
            EQ_notch_width[i] = 0;
          }
        else
          {
            EQ_notch_width[i] = 5;
          }
        EQ_notch_index[i] = nearest_x (EQ_notch_default[i]);


        /*  Set the state so that we can save the scene if we need to.  */

        s_set_description (S_NOTCH_GAIN (i), g_strdup_printf ("Reset notch %d", i));
        s_set_value_ns (S_NOTCH_GAIN (i), EQ_notch_gain[i]);
        s_set_value_ns (S_NOTCH_FREQ (i), EQ_notch_default[i]);
        s_set_value_ns (S_NOTCH_FLAG (i), (float) EQ_notch_flag[i]);
        s_set_value_ns (S_NOTCH_Q (i), (float) EQ_notch_width[i]);
      }


    /*  Set the GEQ.  */

    set_EQ ();


    /*  Redraw the EQ curve.  */

//    draw_EQ_curve ();
	gtk_widget_queue_draw(GTK_WIDGET(l_EQ_curve)); 

    /*  Set the scene warning button so that people will know to save it.  */

    set_scene_warning_button ();
}


/*  Place the sliding notch filters in the hand drawn EQ curve.  */

static void insert_notch ()
{
    int        i, j, ndx, left, right, length, slide;
    float      x[5], y[5];


    /*  Place the interpolated (splined) X and Y data into the "notched" array.
        The interp arrays are the EQ curve that we will "slide" the notches on.
        The "notched" arrays are the EQ with the notches in place.  */

    for (i = 0 ; i < EQ_length ; i++)
      {
        EQ_x_notched[i] = EQ_xinterp[i];
        EQ_y_notched[i] = EQ_yinterp[i];
      }


    for (j = 0 ; j < NOTCHES ; j++)
      {
        /*  We only want to compute the "notched" curves for those notches that
            have their notch flag (active flag) set.  */

        if (EQ_notch_flag[j])
          {
            /*  If j is zero this is the low-shelving notch which behaves
                differently from the normal notches.  */

            if (!j)
              {
                ndx = EQ_notch_index[j];
                slide = MAX (0, (ndx - NOTCH_PASS_WIDTH));

                for (i = 0 ; i < slide ; i++)
                    EQ_y_notched[i] = EQ_notch_gain[j];

                x[0] = EQ_x_notched[slide];
                y[0] = EQ_notch_gain[j];
                x[1] = EQ_x_notched[slide + 1];
                y[1] = EQ_notch_gain[j];
                x[2] = EQ_x_notched[ndx - 1];
                y[2] = EQ_y_notched[ndx - 1];
                x[3] = EQ_x_notched[ndx];
                y[3] = EQ_y_notched[ndx];

                interpolate (EQ_interval, 4, x[0], x[3], &length, x, 
                    y, &EQ_x_notched[slide], &EQ_y_notched[slide]);
              }


            /*  This is the high-shelving notch.  */

            else if (j == NOTCHES - 1)
              {
                ndx = EQ_notch_index[j];
                slide = MIN ((ndx + NOTCH_PASS_WIDTH), (EQ_length - 1));

                for (i = slide ; i < EQ_length ; i++)
                    EQ_y_notched[i] = EQ_notch_gain[j];

                x[0] = EQ_x_notched[ndx];
                y[0] = EQ_y_notched[ndx];
                x[1] = EQ_x_notched[ndx + 1];
                y[1] = EQ_y_notched[ndx + 1];
                x[2] = EQ_x_notched[slide - 1];
                y[2] = EQ_notch_gain[j];
                x[3] = EQ_x_notched[slide];
                y[3] = EQ_notch_gain[j];

                interpolate (EQ_interval, 4, x[0], x[3], &length, x, 
                    y, &EQ_x_notched[ndx], &EQ_y_notched[ndx]);
              }


            /*  The "normal" notches.  */

            else
              {
                left = EQ_notch_index[j] - EQ_notch_width[j];
                right = EQ_notch_index[j] + EQ_notch_width[j];

                x[0] = EQ_x_notched[left];
                y[0] = EQ_y_notched[left];
                x[1] = EQ_x_notched[left + 1];
                y[1] = EQ_y_notched[left + 1];
                x[2] = EQ_x_notched[EQ_notch_index[j]];
                y[2] = EQ_notch_gain[j];
                x[3] = EQ_x_notched[right - 1];
                y[3] = EQ_y_notched[right - 1];
                x[4] = EQ_x_notched[right];
                y[4] = EQ_y_notched[right];

                interpolate (EQ_interval, 5, x[0], x[4], &length, x, 
                    y, &EQ_x_notched[left], &EQ_y_notched[left]);
              }
          }
      }
}


/*  Draw the EQ curve.  This may be from the graphic EQ sliders if they have
    been modified.  Usually from the hand drawn EQ though.  */

void draw_EQ_curve (cairo_t *EQ_cr)
{
    int            i, x0 = 0, y0 = 0, x1, y1, inc;
    float          x[EQ_BANDS], y[EQ_BANDS];
 
    /*  If the EQ widget has not been realized (not visible), go away.  */

    if (!EQ_realized) return;

	
    /*  Clear the curve drawing area.  */

    color = get_color (HDEQ_BACKGROUND_COLOR);
    cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);

    cairo_rectangle (EQ_cr, 0, 0, EQ_curve_width + 1, EQ_curve_height + 1);
    cairo_fill_preserve (EQ_cr);
    
    color = get_color (HDEQ_GRID_COLOR);
	cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);

	cairo_set_line_width (EQ_cr, 1.0);
	cairo_stroke (EQ_cr);

    /*  Draw the grid lines.  First we get the latest and greatest GEQ gains
        and frequencies.  */

    geq_get_freqs_and_gains (l_geq_freqs, l_geq_gains);


    /*  Frequency lines on log scale in X.  */

 //   gdk_gc_set_line_attributes (EQ_gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT,
 //       GDK_JOIN_MITER);
    i = ((int) (l_geq_freqs[0] + 10.0) / 10) * 10;
    inc = 10;
    while (i < l_geq_freqs[EQ_BANDS - 1])
      {
        for (x0 = i ; x0 <= inc * 10 ; x0 += inc)
          {
				freq2xpix ((float) x0, &x1);

				cairo_move_to (EQ_cr, x1, 0); 
				cairo_line_to (EQ_cr, x1, EQ_curve_height);
				cairo_stroke (EQ_cr);

          }
        i = inc * 10;
        inc *= 10;
      } 
        

    /*  Gain lines in Y.  */

    inc = 10;
    if (EQ_curve_range_y < 10.0) inc = 1;
    for (i = NINT (EQ_gain_lower) ; i < NINT (EQ_gain_upper) ; i++)
      {
        if (!(i % inc))
          {
				gain2ypix ((float) i, &y1);

				cairo_move_to (EQ_cr, 0, y1); 
				cairo_line_to (EQ_cr, EQ_curve_width, y1);
				cairo_stroke (EQ_cr);
          }
      }


    /*  Add the crossover bars.  */

 //   gdk_gc_set_line_attributes (EQ_gc, 2, GDK_LINE_SOLID, GDK_CAP_BUTT,
 //       GDK_JOIN_MITER);


    color = get_color (LOW_BAND_COLOR);
	cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue); 
	
    freq2xpix (process_get_low2mid_xover (), &x1);
    

		cairo_move_to (EQ_cr, x1, 0); 
		cairo_line_to (EQ_cr, x1, EQ_curve_height);
		cairo_stroke (EQ_cr);
 
		cairo_rectangle (EQ_cr, x1 - XOVER_HANDLE_HALF_SIZE, 0, XOVER_HANDLE_SIZE, XOVER_HANDLE_SIZE);
		cairo_fill (EQ_cr);  
 
 		cairo_rectangle (EQ_cr, x1 - XOVER_HANDLE_HALF_SIZE, EQ_curve_height - XOVER_HANDLE_SIZE, XOVER_HANDLE_SIZE, XOVER_HANDLE_SIZE);
		cairo_fill (EQ_cr);

		
    xover_handle_l2m = x1;


 
    color = get_color (HIGH_BAND_COLOR);
	cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);  
    freq2xpix (process_get_mid2high_xover (), &x1);
 
  		cairo_move_to (EQ_cr, x1, 0); 
  		cairo_line_to (EQ_cr, x1, EQ_curve_height);
		cairo_stroke (EQ_cr);

  		cairo_rectangle (EQ_cr, x1 - XOVER_HANDLE_HALF_SIZE, 0, XOVER_HANDLE_SIZE, XOVER_HANDLE_SIZE);
		cairo_fill (EQ_cr); 

 		cairo_rectangle (EQ_cr, x1 - XOVER_HANDLE_HALF_SIZE, EQ_curve_height - XOVER_HANDLE_SIZE, XOVER_HANDLE_SIZE, XOVER_HANDLE_SIZE);
		cairo_fill (EQ_cr); 


    xover_handle_m2h = x1;

    
    /*  If we've messed with the graphics EQ sliders, recompute the splined 
        curve.  See hdeq_eqb_mod above.  */

    if (EQ_mod) 
      {
        /*  Set X and Y arrays of the 31 bands from the GEQ.  */

        for (i = 0 ; i < EQ_BANDS ; i++)
          {
            x[i] = log10 (l_geq_freqs[i]);
            y[i] = log10 (l_geq_gains[i]);
          }


        /*  Spline the bands to 1024 points.  */

        interpolate (EQ_interval, EQ_BANDS, EQ_start, EQ_end, 
                     &EQ_length, x, y, EQ_xinterp, EQ_yinterp);


        /*  Save state of the EQ curve (for scene changes, etc).  */

        s_set_value_block (EQ_yinterp, S_EQ_GAIN(0), EQ_length);


        /*  Reset all of the shelves/notches.  */


        for (i = 0 ; i < NOTCHES ; i++)
          {
            EQ_notch_flag[i] = 0;
            EQ_notch_gain[i] = 0.0;
            EQ_notch_index[i] = nearest_x (EQ_notch_default[i]);

            if (!i || i == NOTCHES - 1)
              {
                EQ_notch_width[i] = 0;
              }
            else
              {
                EQ_notch_width[i] = 5;
              }
          }


        /*  Insert the notches before we pull out the frequencies and save 
            state.  */

        insert_notch ();


        /*  Save the state.  */

        for (i = 0 ; i < NOTCHES ; i++)
          {
			s_set_description (S_NOTCH_GAIN (i), g_strdup_printf("Reset notch %d", i));
            s_set_value_ns (S_NOTCH_GAIN (i), EQ_notch_gain[i]);
            s_set_value_ns (S_NOTCH_Q (i), (float) EQ_notch_width[i]);
            s_set_value_ns (S_NOTCH_FREQ (i), powf (10.0f, EQ_x_notched[EQ_notch_index[i]]));
            s_set_value_ns (S_NOTCH_FLAG (i), (float) EQ_notch_flag[i]);
          }
           
      }


    /*  Plot the curve.  Note that we're plotting the "notched" arrays not
        the interp arrays.  */

	color = get_color (HDEQ_CURVE_COLOR);
	cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);    
	
	
    for (i = 0 ; i < EQ_length - 1 ; i++)
      {
        logfreq2xpix (EQ_x_notched[i], &x1);
        loggain2ypix (EQ_y_notched[i], &y1);

		if (i){
			
				cairo_move_to (EQ_cr, x0, y0); 
				cairo_line_to (EQ_cr, x1, y1);
				cairo_stroke (EQ_cr);
		}
        x0 = x1;
        y0 = y1;
      }


    /*  Add the notch handles.  */
	//printf("adding notches \n");
	
    for (i = 0 ; i < NOTCHES ; i++)
      {
	//	printf("adding notches %i \n", i);
		color = get_color (HANDLE_COLOR);
		cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);
		
        logfreq2xpix (EQ_x_notched[EQ_notch_index[i]], &x1);


        /*  Make the shelf handles follow the shelf instead of staying with
            the EQ curve.  */

        if (EQ_notch_flag[i] && (!i || i == NOTCHES - 1))
          {
            loggain2ypix (EQ_notch_gain[i], &y1);
          }
        else
          {
            loggain2ypix (EQ_y_notched[EQ_notch_index[i]], &y1);
          }

	//	printf("adding notches %i %i %i \n", i, x1, y1);
		cairo_arc (EQ_cr, x1, y1, NOTCH_HANDLE_WIDTH/2, 0, 2 * M_PI);
		cairo_fill_preserve (EQ_cr);

		color = get_color (HIGH_BAND_COLOR);
		cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);  
		cairo_set_line_width (EQ_cr, 1.0);
		cairo_stroke (EQ_cr);


        EQ_notch_handle[0][0][i] = EQ_notch_handle[0][1][i] = 
            EQ_notch_handle[0][2][i]= x1;
        EQ_notch_handle[1][1][i] = y1;

        if (!i)
          {
            EQ_notch_handle[0][0][i] = 0;
          }
        else
          {
            EQ_notch_handle[0][2][i] = EQ_curve_width;
          }


        /*  Notch handles, not shelf.  */

        if (i && i < NOTCHES - 1)
          {

			color = get_color (HANDLE_COLOR);
			cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);
			
            x0 = EQ_notch_index[i] - EQ_notch_width[i];

            logfreq2xpix (EQ_x_notched[x0], &x1);
            loggain2ypix (EQ_y_notched[x0], &y1);

            if (EQ_notch_handle[0][1][i] - x1 < NOTCH_HANDLE_HALF_WIDTH) 
                x1 = EQ_notch_handle[0][1][i] - NOTCH_HANDLE_WIDTH;
			cairo_rectangle (EQ_cr, x1 - NOTCH_HANDLE_HALF_WIDTH/2, y1 - NOTCH_CENTER_HALF_HEIGHT/3, NOTCH_HANDLE_WIDTH/2, NOTCH_CENTER_HEIGHT/3);
			cairo_fill_preserve (EQ_cr); 
			color = get_color (HIGH_BAND_COLOR);
			cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);  
			cairo_set_line_width (EQ_cr, 1.0);
			cairo_stroke (EQ_cr);
	
            EQ_notch_handle[0][0][i] = x1;
            EQ_notch_handle[1][0][i] = y1;


			color = get_color (HANDLE_COLOR);
			cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);
			
            x0 = EQ_notch_index[i] + EQ_notch_width[i];

            logfreq2xpix (EQ_x_notched[x0], &x1);
            loggain2ypix (EQ_y_notched[x0], &y1);

            if (x1 - EQ_notch_handle[0][1][i] < NOTCH_HANDLE_HALF_WIDTH) 
                x1 = EQ_notch_handle[0][1][i] + NOTCH_HANDLE_WIDTH;

			cairo_rectangle (EQ_cr, x1 - NOTCH_HANDLE_HALF_WIDTH/2, y1 - NOTCH_CENTER_HALF_HEIGHT/3, NOTCH_HANDLE_WIDTH/2, NOTCH_CENTER_HEIGHT/3);

			cairo_fill_preserve (EQ_cr);
			color = get_color (HIGH_BAND_COLOR);
			cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);  
			cairo_set_line_width (EQ_cr, 1.0);
			cairo_stroke (EQ_cr);		
				
            EQ_notch_handle[0][2][i] = x1;
            EQ_notch_handle[1][2][i] = y1;
          }
      }

//    gdk_gc_set_line_attributes (EQ_gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT,
//        GDK_JOIN_MITER);


    EQ_mod = 0;

/* Draw EQ Spectrum Curve */
		color = get_color (HDEQ_SPECTRUM_COLOR);
		cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);
		cairo_move_to (EQ_cr, spectrum_x[0], spectrum_y[0]); 
		for (i = 0 ; i < EQ_INTERP ; i++)
		{
			if (i) {
						
				cairo_line_to (EQ_cr, spectrum_x[i], spectrum_y[i]);			
			    
			}
		}	
		cairo_stroke (EQ_cr);

	/* Draw hand drawn curve */	
/*	if (EQ_drawing){
		
        color = get_color (HDEQ_CURVE_COLOR);
		cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);
		cairo_move_to (EQ_cr, EQ_xinput[0], EQ_yinput[0]);
		for (i = 0 ; i < EQ_input_points ; i++)
        {
		
			//cairo_move_to (EQ_cr, EQ_xinput[i], EQ_yinput[i]);
			cairo_line_to (EQ_cr,  EQ_xinput[i], EQ_yinput[i]);
        }                                              
 //                   gdk_draw_line (hdeq_pixmap, EQ_gc, 
 //                                  NINT (EQ_xinput[EQ_input_points - 1]), 
 //                                  NINT (EQ_yinput[EQ_input_points - 1]), x, y);
	}
*/

}


/*  Whenever the curve is exposed, which will happen on a resize, we need to
    get the current dimensions and redraw the curve.  */

void hdeq_curve_draw (GtkWidget *widget, cairo_t *cr, gpointer data)
{
    /*  We're using the upper and lower ranges of the crossovers to get the
        X range.  */

    l_low2mid_adj = gtk_range_get_adjustment ((GtkRange *) l_low2mid);
    EQ_curve_range_x = gtk_adjustment_get_upper (l_low2mid_adj) - gtk_adjustment_get_lower (l_low2mid_adj);

//	printf("%f, %f\n", gtk_adjustment_get_upper (l_low2mid_adj), gtk_adjustment_get_lower (l_low2mid_adj));
	
    EQ_curve_range_y = EQ_gain_upper - EQ_gain_lower;


    /*  Since allocation width and height are inclusive we need to decrement
        for calculations.  */
    
    GtkAllocation *allocation = g_new0 (GtkAllocation, 1);

	gtk_widget_get_allocation(GTK_WIDGET(widget), allocation);

    EQ_curve_width = allocation->width - 1;
    EQ_curve_height = allocation->height - 1;
	g_free (allocation);
	
    draw_EQ_curve (cr);

    /*  Window is ready for motion events.  */

    hdeq_ready = TRUE;
}


/*  Initialize the hdeq.  This comes from the realize callback for the hdeq
    drawing area.  Called from callbacks.c.  */

void hdeq_curve_init (GtkWidget *widget)
{

    EQ_pc = gtk_widget_get_pango_context (widget);

    geq_get_freqs_and_gains (l_geq_freqs, l_geq_gains);

    EQ_start = log10 (l_geq_freqs[0]);
    EQ_end = log10 (l_geq_freqs[EQ_BANDS - 1]);
    EQ_interval = (EQ_end - EQ_start) / EQ_INTERP;


    /*  Setting a callback based on notch gain changes.  */

    s_set_callback (S_NOTCH_GAIN(0), set_EQ_curve_values);
	
    EQ_realized = 1;

}

void hdeq_curve_update()
{
	
		gtk_widget_queue_draw(GTK_WIDGET(l_EQ_curve)); 
	
}

/*  Don't let the notches overlap.  */

/*  notch is the notch number (0-4), new is the nearest frequency index to the cursor, 
    q is 0 for center of notch, 1 for left handle, 2 for right handle.  */

static int check_notch (int notch, int new, int q)
{
    int         j, k, left, right, width, ret;


    ret = 1;


    /*  Left shelf.  */

    if (!notch)
      {
        j = EQ_notch_index[notch + 1] - EQ_notch_width[notch + 1];
        if (new >= j || new < 10) ret = 0;
      }


    /*  Right shelf.  */

    else if (notch == NOTCHES - 1)
      {
        k = EQ_notch_index[notch - 1] + EQ_notch_width[notch - 1];
        if (new <= k || new > EQ_length - 10) ret = 0;
      }


    /*  Notches.  */

    else
      {
        j = EQ_notch_index[notch - 1];
        k = EQ_notch_index[notch + 1];


        /*  Left handle  */

        if (q == 1)
          {
            left = new;
            width = EQ_notch_index[notch] - left;
            right = left + 2 * width;

            if (EQ_notch_index[notch] - left < 5) ret = 0;
          }


        /*  Right handle  */

        else if (q == 2)
          {
            right = new;
            width = right - EQ_notch_index[notch];
            left = right - 2 * width;

            if (right - EQ_notch_index[notch] < 5) ret = 0;
          }


        /*  Center handle  */

        else
          {
            left = new - EQ_notch_width[notch];
            right = new + EQ_notch_width[notch];
          }
        if (left <= j || right >= k) ret = 0;
      }

    return (ret);
}


/*  This comes from the hdeq drawing area motion callback (actually the event
    box).  There are about a million things going on here.  This is basically
    the engine for the hdeq interface.  The rest of it happens in the button
    press and release handlers.  Take a look at the comments in the function 
    to see what's actually happening.  */

void hdeq_curve_motion (GdkEventMotion *event)
{
    static int      prev_x = -1, prev_y = -1, current_cursor = -1;
    int             i, j, x, y, size, diffx_l2m, diffx_m2h, diff_notch[2], 
                    cursor, drag, notch_flag = -1, lo, hi, clock_diff;
    float           freq, gain, s_gain;
    char            *coords = NULL;
    clock_t         new_clock;
    static clock_t  old_clock = -1;
    struct tms      buf;
    static gboolean modified;


    /*  Unset the drawing flag.  If we modify the background in any way we set this flag so
        that we can paint the pixmap back to the screen at the end of this function.  We
        will also paint the last spectrum curve to the widget itself so that the user can
        see  the last state of the spectrum curve while modifying the hdeq.  */

    modified = FALSE;


    /*  We don't want motion events until the window is ready. */

    if (!hdeq_ready) return;


    /*  Timing delay so we don't get five bazillion callbacks.  */

    new_clock = times (&buf);
    clock_diff = abs (new_clock - old_clock);

    if (clock_diff < MOTION_CLOCK_DIFF) return;

    old_clock = new_clock;


    x = NINT (event->x);
    y = NINT (event->y);
    drag = 0;


    /*  We only want to update things if we've actually moved the cursor.  */

    if (x != prev_x || y != prev_y)
      {
        /*  Set the "caption" for the window.  We're tracking the cursor in
            relation to frequency, EQ gain, and spectrum curve gain.  */

        freq = pow (10.0, (gtk_adjustment_get_lower (l_low2mid_adj) + (((double) x / 
            (double) EQ_curve_width) * EQ_curve_range_x)));

        gain = ((((double) EQ_curve_height - (double) y) / 
            (double) EQ_curve_height) * EQ_curve_range_y) + 
            EQ_gain_lower;

        s_gain = -(EQ_SPECTRUM_RANGE - (((((double) EQ_curve_height - 
            (double) y) / (double) EQ_curve_height) * EQ_SPECTRUM_RANGE)));

	coords = g_strdup_printf(_("%dHz , EQ : %.1fdb , Spectrum : %ddb"), NINT
		(freq), gain, NINT (s_gain));

        gtk_label_set_text (l_EQ_curve_lbl, coords);
	free(coords);


        /*  If we're in the midst of drawing the curve...  We're going to 
            build the EQ_input arrays from the cursor track.  */

        if (EQ_drawing && EQ_input_points)
          {
            /*  Make sure that we moved in the X direction so we can figure out
                which direction we're drawing in.  */

            if (x != EQ_xinput[EQ_input_points - 1])
              {
                if (!EQ_draw_dir)
                  {
                    if (x < EQ_xinput[EQ_input_points - 1])
                      {
                        EQ_draw_dir = -1;
                      }
                    else
                      {
                        EQ_draw_dir = 1;
                      }
                  }


                /*  Only grab the point if we're going in the proper direction
                    based on the first two points drawn.  */

                if ((EQ_draw_dir == 1 && x > EQ_xinput[EQ_input_points - 1]) ||
                    (EQ_draw_dir == -1 && x < EQ_xinput[EQ_input_points - 1]))
                  {
 //                   gdk_gc_set_foreground (EQ_gc, get_color (HDEQ_CURVE_COLOR));
              //      color = get_color (HDEQ_CURVE_COLOR);
				//	cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);
                                                      
 //                   gdk_draw_line (hdeq_pixmap, EQ_gc, 
 //                                  NINT (EQ_xinput[EQ_input_points - 1]), 
 //                                  NINT (EQ_yinput[EQ_input_points - 1]), x, y);
 //                   gdk_gc_set_foreground (EQ_gc, get_color (TEXT_COLOR));
			//		color = get_color (TEXT_COLOR);
			//		cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);
					
                    size = (EQ_input_points + 1) * sizeof (float);
                    EQ_xinput = (float *) realloc (EQ_xinput, size);
                    EQ_yinput = (float *) realloc (EQ_yinput, size);

                    if (EQ_yinput == NULL)
                      {
                        perror (_("Allocating EQ_yinput in callbacks.c"));
                        clean_quit ();
                      }

                    EQ_xinput[EQ_input_points] = (float) x;
                    EQ_yinput[EQ_input_points] = (float) y;
                    EQ_input_points++;


                    /*  Set the modified flag.  */

                    modified = TRUE;
                  }
              }
          }


        /*  We're dragging the low to mid crossover in the HDEQ.  */

        else if (EQ_drag_l2m)
          {
            freq = log10f (freq);
            gtk_range_set_value ((GtkRange *) l_low2mid, freq);


            /*  Set the modified flag.  */

            modified = TRUE;
          }


        /*  We're dragging the mid to high crossover in the HDEQ.  */

        else if (EQ_drag_m2h)
          {
            freq = log10f (freq);
            gtk_range_set_value ((GtkRange *) l_mid2high, freq);


            /*  Set the modified flag.  */

            modified = TRUE;
          }


        /*  We're just moving the cursor in the window or we're dragging the
            notches around.  */

        else
          {
            notch_flag = -1;


            /*  Check for notch drag.  */

            for (i = 0 ; i < NOTCHES ; i++)
              {
                if (EQ_notch_drag[i])
                  {
                    /*  If we're shifted we're raising or lowering notch 
                        gain only.  */

                    if (event->state & GDK_SHIFT_MASK)
                      {
                        if (y >= 0 && y <= EQ_curve_height)
                          {
                            EQ_notch_gain[i] = (((((double) EQ_curve_height - 
                                (double) y) / (double) EQ_curve_height) * 
                                EQ_curve_range_y) + EQ_gain_lower) * 0.05;

                            drag = 1;
                            notch_flag = i;
                            EQ_notch_flag[i] = 1;


                            /*  Save state.  */

							s_set_description (S_NOTCH_GAIN (i), g_strdup_printf("Move notch %d", i));
                            s_set_value_ns (S_NOTCH_GAIN (i), EQ_notch_gain[i]);
                            s_set_value_ns (S_NOTCH_FREQ (i), freq);
                            s_set_value_ns (S_NOTCH_FLAG (i), (float) EQ_notch_flag[i]);
                            s_set_value_ns (S_NOTCH_Q (i), (float) EQ_notch_width[i]);

                            break;
                          }
                      }


                    /*  Dragging the notch handle in X and Y (i.e. not 
                        shifted).  */

                    else
                      {
                        if (x >= 0 && x <= EQ_curve_width && y >= 0 && 
                            y <= EQ_curve_height)
                          {
                            j = nearest_x (freq);
                            if (check_notch (i, j, 0))
                              {
                                EQ_notch_index[i] = nearest_x (freq);

                                EQ_notch_gain[i] = 
                                    (((((double) EQ_curve_height - 
                                    (double) y) / (double) EQ_curve_height) * 
                                    EQ_curve_range_y) + EQ_gain_lower) * 
                                    0.05;
                                EQ_notch_flag[i] = 1;

                                drag = 1;
                                notch_flag = i;


                                /*  Save state.  */

								s_set_description (S_NOTCH_GAIN (i), g_strdup_printf("Move notch %d", i));
                                s_set_value_ns (S_NOTCH_GAIN (i), EQ_notch_gain[i]);
                                s_set_value_ns (S_NOTCH_FREQ (i), freq);
                                s_set_value_ns (S_NOTCH_FLAG (i), (float) EQ_notch_flag[i]);
                                s_set_value_ns (S_NOTCH_Q (i), (float) EQ_notch_width[i]);
                              }
                            break;
                          }
                      }
                  }


                /*  Dragging the Q/width handles for the notch filters.  */

                if (EQ_notch_Q_drag[i])
                  {
                    if (x >= 0 && x <= EQ_curve_width)
                      {
                        j = nearest_x (freq);
                        if (check_notch (i, j, EQ_notch_Q_drag[i]))
                          {
                            /*  Left side is set to 1...  */

                            if (EQ_notch_Q_drag[i] == 1)
                              {
                                EQ_notch_width[i] = EQ_notch_index[i] - j;
                              }


                            /*  Right side is set to 2...  */

                            else
                              {
                                EQ_notch_width[i] = j - EQ_notch_index[i];
                              }

                            drag = 1;
                            notch_flag = i;


                            /*  Save state.  */

							s_set_description (S_NOTCH_GAIN (i), g_strdup_printf("Move notch %d", i));
                            s_set_value_ns (S_NOTCH_GAIN (i), EQ_notch_gain[i]);
                            s_set_value_ns (S_NOTCH_FREQ (i), freq);
                            s_set_value_ns (S_NOTCH_FLAG (i), (float) EQ_notch_flag[i]);
                            s_set_value_ns (S_NOTCH_Q (i), (float) EQ_notch_width[i]);
                          }
                        break;
                      }
                  }
              }


            /*  If we're dragging (drag set above) a notch filter...  */

            if (drag)
              {
                /*  Set the new notches and redraw everything.  */

                insert_notch ();
                set_EQ ();
            //    draw_EQ_curve ();


                /*  Set the modified flag.  */

                modified = TRUE;
              }


            /*  Just moving the cursor...  */

            else
              {
                /*  If we pass over any of the handles we want to change the
                    cursor.  We check by comparing the X/Y cursor position
                    to the X/Y position of the handles and their 
                    width/height.  */

                cursor = GDK_PENCIL;

                if (EQ_drag_l2m || EQ_drag_m2h) cursor = GDK_SB_H_DOUBLE_ARROW;

                diffx_l2m = abs (x - xover_handle_l2m);
                diffx_m2h = abs (x - xover_handle_m2h);


                /*  No point in checking for notches if we're already passing 
                    over one of the xover bars.  */

                if ((diffx_l2m <= XOVER_HANDLE_HALF_SIZE || diffx_m2h <= XOVER_HANDLE_HALF_SIZE) &&
                    (y <= XOVER_HANDLE_SIZE || y >= EQ_curve_height - XOVER_HANDLE_SIZE)) 
                  {
                    cursor = GDK_SB_H_DOUBLE_ARROW;
                  }
                else
                  {
                    for (i = 0 ; i < NOTCHES ; i++)
                      {
                        if (EQ_notch_drag[i] || EQ_notch_Q_drag[i])
                          {
                            /*  Shift is pressed so we can only adjust gain,
                                therefore we want the vertical double 
                                arrow.  */

                            if (event->state & GDK_SHIFT_MASK)
                              {
                                cursor = GDK_SB_V_DOUBLE_ARROW;
                              }
                            else
                              {
                                /*  Cross or horizontal double arrow depending
                                    on whether we are over a notch handle or a
                                    Q handle.  */

                                if (EQ_notch_drag[i])
                                  {
                                    cursor = GDK_CROSS;
                                  }
                                else
                                  {
                                    cursor = GDK_SB_H_DOUBLE_ARROW;
                                  }
                              }
                            notch_flag = i;
                            break;
                          }

                        diff_notch[0] = abs (x - EQ_notch_handle[0][1][i]);
                        diff_notch[1] = abs (y - EQ_notch_handle[1][1][i]);

                        if (diff_notch[0] <= NOTCH_HANDLE_HALF_WIDTH &&
                            diff_notch[1] <= NOTCH_CENTER_HEIGHT)
                          {
                            /*  Shift is pressed so we can only adjust gain,
                                therefore we want the vertical double 
                                arrow.  */

                            if (event->state & GDK_SHIFT_MASK)
                              {
                                cursor = GDK_SB_V_DOUBLE_ARROW;
                              }
                            else
                              {
                                cursor = GDK_CROSS;
                              }
                            notch_flag = i;
                            break;
                          }


                        /*  Only the "normal" handles.  */

                        if (i && i < NOTCHES - 1)
                          {
                            diff_notch[0] = abs (x - EQ_notch_handle[0][0][i]);
                            diff_notch[1] = abs (y - EQ_notch_handle[1][0][i]);

                            if (diff_notch[0] <= NOTCH_HANDLE_HALF_WIDTH &&
                                diff_notch[1] <= NOTCH_HANDLE_HALF_HEIGHT)
                              {
                                cursor = GDK_SB_H_DOUBLE_ARROW;
                                notch_flag = i;
                                break;
                              }


                            diff_notch[0] = abs (x - EQ_notch_handle[0][2][i]);
                            diff_notch[1] = abs (y - EQ_notch_handle[1][2][i]);

                            if (diff_notch[0] <= NOTCH_HANDLE_HALF_WIDTH &&
                                diff_notch[1] <= NOTCH_HANDLE_HALF_HEIGHT)
                              {
                                cursor = GDK_SB_H_DOUBLE_ARROW;
                                notch_flag = i;
                                break;
                              }
                          }
                      }
                  }


                /*  Only set the cursor if it changes.  */

                if (current_cursor != cursor)
                  {
                    current_cursor = cursor;
  //                  gdk_window_set_cursor (EQ_drawable, gdk_cursor_new (cursor));
                  }
              }


            /*  Change the "caption" if we are over a handle.  */

            if (notch_flag != -1)
              {
                i = EQ_notch_index[notch_flag] - EQ_notch_width[notch_flag];
                if (i < 0 || notch_flag == 0) i = 0;
                j = EQ_notch_index[notch_flag] + EQ_notch_width[notch_flag];
                if (j >= EQ_length || notch_flag == NOTCHES - 1) j = EQ_length - 1;
                lo = NINT (pow (10.0, EQ_xinterp[i]));
                hi = NINT (pow (10.0, EQ_xinterp[j]));

                coords = g_strdup_printf (_("%ddb , %dHz - %dHz"), NINT (gain), lo, hi);
                gtk_label_set_text (l_EQ_curve_lbl, coords);
		free(coords);
              }
          }


        /*  Save the previous pixel position.  */

        prev_x = x;
        prev_y = y;



        /*  If we drew anything, blast the background pixmap to the widget and paint the last
            spectrum curve (if spectrum update frequency isn't 0).  */

        if (modified)
          {
          //  gdk_draw_pixmap (EQ_drawable, EQ_gc, hdeq_pixmap, 0, 0, 0, 0,
          //                   EQ_curve_width + 1, EQ_curve_height + 1);
			
			gtk_widget_queue_draw(GTK_WIDGET(l_EQ_curve)); 

 //           if (get_spectrum_freq ())
 //             {
                /*  Set the foreground color for drawing the spectrum curve.  */

 //               gdk_gc_set_foreground (EQ_gc, get_color (HDEQ_SPECTRUM_COLOR));
 //               color = get_color (HDEQ_SPECTRUM_COLOR);
//				cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);

 //               for (i = 0 ; i < EQ_INTERP ; i++)
  //                {
  //                  if (i) gdk_draw_line (EQ_drawable, EQ_gc, spectrum_x[i - 1], spectrum_y[i - 1],
  //                                        spectrum_x[i], spectrum_y[i]);
  //                }


                /*  Reset the foreground color to the text color.  */

//                gdk_gc_set_foreground (EQ_gc, get_color (TEXT_COLOR));
 //               color = get_color (TEXT_COLOR);
//				cairo_set_source_rgb (EQ_cr,color->red,color->green, color->blue);
 //             }
          }
      }
}


/*  This comes from the hdeq drawing area button press callback (actually the 
    event box).  Again, many things happening here depending on the location
    of the cursor when the button is pressed.  Take a look at the comments in 
    the function to see what's actually happening.  */

void hdeq_curve_button_press (GdkEventButton *event)
{
    float               *x = NULL, *y = NULL;
    int                 diffx_l2m, diffx_m2h, diff_notch[2], i, j, i_start = 0, 
                        i_end = 0, size, ex, ey, *temp_x = NULL, 
                        *temp_y = NULL;
    static int          interp_pad = 5;


    ex = event->x;
    ey = event->y;

    switch (event->button)
      {

        /*  Button 1 or 2 - start drawing or end drawing unless we're over a notch
            or xover handle in which case we will be grabbing and sliding the
            handle in the X or X/Y direction(s).  <Shift> button 1 or 2 is for 
            grabbing and sliding only in the Y direction (notch/shelf filters 
            only - look at the motion callback).  <Ctrl> button 1 or 2 will reset 
            shelf and notch values to 0.0.  If you use button 2 you will only
            be able to drag handles, not draw the curve.  This can make life a bit 
            easier since you won't start drawing a curve if you miss the handle.  */

      case 1:
      case 2:

        /*  Start drawing.  */

        if (!EQ_drawing)
          {
            /*  Checking for position over xover bar or notch handles.  We 
                check by comparing the X/Y cursor position to the X/Y position
                of the handles and their width/height.  */

            diffx_l2m = abs (ex - xover_handle_l2m);
            diffx_m2h = abs (ex - xover_handle_m2h);


            /*  Over low to mid crossover handle.  */

            if (diffx_l2m <= XOVER_HANDLE_HALF_SIZE && 
                (ey <= XOVER_HANDLE_SIZE ||
                 ey >= EQ_curve_height - XOVER_HANDLE_SIZE))
              {
                EQ_drag_l2m = 1;
                xover_active = 1;
              }


            /*  Over mid to high crossover handle.  */

            else if (diffx_m2h <= XOVER_HANDLE_HALF_SIZE && 
                (ey <= XOVER_HANDLE_SIZE || 
                 ey >= EQ_curve_height - XOVER_HANDLE_SIZE))
              {
                EQ_drag_m2h = 1;
                xover_active = 1;
              }


            /*  Anywhere else.  */

            else
              {
                /*  Check the notches.  */

                for (i = 0 ; i < NOTCHES ; i++)
                  {

                    /*  Check the center of the notches first  */

                    diff_notch[0] = abs (ex - EQ_notch_handle[0][1][i]);
                    diff_notch[1] = abs (ey - EQ_notch_handle[1][1][i]);

                    if (diff_notch[0] <= NOTCH_HANDLE_HALF_WIDTH &&
                        diff_notch[1] <= NOTCH_CENTER_HALF_HEIGHT)
                      {
                        /*  Reset if <Ctrl> is pressed.  */

                        xover_active = 1;
                        if (event->state & GDK_CONTROL_MASK)
                          {
                            EQ_notch_flag[i] = 0;
                            EQ_notch_gain[i] = 0.0;

                            if (!i || i == NOTCHES - 1)
                              {
                                EQ_notch_width[i] = 0;
                              }
                            else
                              {
                                EQ_notch_width[i] = 5;
                              }


                            /*  Save state.  */

			    s_set_description (S_NOTCH_GAIN (i), g_strdup_printf("Reset notch %d", i));
                            s_set_value_ns (S_NOTCH_GAIN (i), EQ_notch_gain[i]);
                            s_set_value_ns (S_NOTCH_Q (i), (float) EQ_notch_width[i]);
                            s_set_value_ns (S_NOTCH_FLAG (i), (float) EQ_notch_flag[i]);


                            /*  Recompute the "notched" curves and redraw.  */

                            insert_notch ();
                            set_EQ ();
                   //         draw_EQ_curve ();
							gtk_widget_queue_draw(GTK_WIDGET(l_EQ_curve)); 
                          }
                        else
                          {
                            EQ_notch_drag[i] = 1;
                          }
                        break;
                      }


                    /*  "Normal" (non-shelving) notch handles.  */

                    if (i && i < NOTCHES - 1)
                      {

                        /*  Check left handle  */

                        diff_notch[0] = abs (ex - EQ_notch_handle[0][0][i]);
                        diff_notch[1] = abs (ey - EQ_notch_handle[1][0][i]);

                        if (diff_notch[0] <= NOTCH_HANDLE_HALF_WIDTH &&
                            diff_notch[1] <= NOTCH_HANDLE_HALF_HEIGHT)
                          {
                            /*  Left bracket is a 1.  */

                            EQ_notch_Q_drag[i] = 1;
                            xover_active = 1;

                            break;
                          }


                        /*  Check right handle  */

                        diff_notch[0] = abs (ex - EQ_notch_handle[0][2][i]);
                        diff_notch[1] = abs (ey - EQ_notch_handle[1][2][i]);

                        if (diff_notch[0] <= NOTCH_HANDLE_HALF_WIDTH &&
                            diff_notch[1] <= NOTCH_HANDLE_HALF_HEIGHT)
                          {
                            /*  Right bracket is a 2.  */

                            EQ_notch_Q_drag[i] = 2;
                            xover_active = 1;

                            break;
                          }
                      }
                  }


                /*  If we aren't over a handle we must be starting to draw 
                    the curve so mark the starting point.  */

                if (!xover_active && event->button == 1)
                  {
                    /*  Save the first point so we can do real narrow EQ 
                        changes.  */

                    size = (EQ_input_points + 1) * sizeof (float);
                    EQ_xinput = (float *) realloc (EQ_xinput, size);
                    EQ_yinput = (float *) realloc (EQ_yinput, size);

                    if (EQ_yinput == NULL)
                      {
                        perror (_("Allocating EQ_yinput in callbacks.c"));
                        clean_quit ();
                      }

                    EQ_xinput[EQ_input_points] = (float) ex;
                    EQ_yinput[EQ_input_points] = (float) ey;
                    EQ_input_points++;

                    EQ_drawing = 1;
                  }
              }
          }


        /*  End drawing - combine the drawn data with any parts of the 
            previous that haven't been superceded by what was drawn.  Use 
            an "interp_pad" cushion on either side of the drawn section 
            so it will merge nicely with the old data.  */

        else if (event->button == 1)
          {
            /*  If the user drew right to left we need to reverse the 
                field.  */

            if (EQ_draw_dir == -1)
              {
                temp_x = (int *) calloc (EQ_input_points, sizeof (int));
                if (temp_x == NULL)
                  {
                    perror ("Allocating direction memory X");
                    exit (-1);
                  }
                temp_y = (int *) calloc (EQ_input_points, sizeof (int));
                if (temp_y == NULL)
                  {
                    perror ("Allocating direction memory Y");
                    exit (-1);
                  }

                for (i = 0 ; i < EQ_input_points ; i++)
                  {
                    temp_x[i] = EQ_xinput[i];
                    temp_y[i] = EQ_yinput[i];
                  }

                for (i = 0, j = EQ_input_points - 1 ; i < EQ_input_points ; 
                     i++, j--)
                  {
                    EQ_xinput[i] = temp_x[j];
                    EQ_yinput[i] = temp_y[j];
                  }

                free (temp_x);
                free (temp_y);
              }


            /*  Reset the drawing direction to none.  */

            EQ_draw_dir = 0;


            /*  Convert the x and y input positions to "real" values.  */

            for (i = 0 ; i < EQ_input_points ; i++)
              {
                EQ_xinput[i] = gtk_adjustment_get_lower (l_low2mid_adj) + (((double) EQ_xinput[i] /
                    (double) EQ_curve_width) * EQ_curve_range_x);


                EQ_yinput[i] = (((((double) EQ_curve_height - 
                    (double) EQ_yinput[i]) / (double) EQ_curve_height) * 
                    EQ_curve_range_y) + EQ_gain_lower) * 0.05;
              }


            /*  Merge the drawn section with the old curve.  We're putting it 
                all into the x and y arrays.  */


            /*  Find the beginning.  */

            for (i = 0 ; i < EQ_length ; i++)
              {
                if (EQ_xinterp[i] >= EQ_xinput[0])
                  {
                    i_start = i - interp_pad;
                    break;
                  }
              }


            /*  Find the end.  */

            for (i = EQ_length - 1 ; i >= 0 ; i--)
              {
                if (EQ_xinterp[i] <= EQ_xinput[EQ_input_points - 1])
                  {
                    i_end = i + interp_pad;
                    break;
                  }
              }


            /*  Set anything prior to the beginning of the drawn section from
                the pre-existing interpolated curve (without notches, these
                get added on later).  */

            j = 0;
            for (i = 0 ; i < i_start ; i++)
              {
                size = (j + 1) * sizeof (float);
                x = (float *) realloc (x, size);
                y = (float *) realloc (y, size);

                if (y == NULL)
                  {
                    perror (_("Allocating y in callbacks.c"));
                    clean_quit ();
                  }

                x[j] = EQ_xinterp[i];
                y[j] = EQ_yinterp[i];
                j++;
              }


            /*  Set from the drawn section.  */

            for (i = 0 ; i < EQ_input_points ; i++)
              {
                size = (j + 1) * sizeof (float);
                x = (float *) realloc (x, size);
                y = (float *) realloc (y, size);

                if (y == NULL)
                  {
                    perror (_("Allocating y in callbacks.c"));
                    clean_quit ();
                  }

                x[j] = EQ_xinput[i];
                y[j] = EQ_yinput[i];
                j++;
              }


            /*  Set anything after the drawn section from the interpolated 
                arrays (without the notches).  */

            for (i = i_end ; i < EQ_length ; i++)
              {
                size = (j + 1) * sizeof (float);
                x = (float *) realloc (x, size);
                y = (float *) realloc (y, size);

                x[j] = EQ_xinterp[i];
                y[j] = EQ_yinterp[i];
                j++;
              }


            /*  Recompute the splined curve in the log(freq) domain for
                plotting the EQ.  */

            interpolate (EQ_interval, j, EQ_start, EQ_end, &EQ_length, x, 
                y, EQ_xinterp, EQ_yinterp);


            if (x) free (x);
            if (y) free (y);


            /*  Save state of the EQ curve.  */

            s_set_value_block (EQ_yinterp, S_EQ_GAIN(0), EQ_length);


            EQ_input_points = 0;


            /*  Replace shelf and notch areas.  */

            insert_notch ();


            /*  Set the GEQ faders and the EQ coefs.  */

            set_EQ ();


            EQ_mod = 0;


            /*  Redraw the curve.  */

     //       draw_EQ_curve ();
			gtk_widget_queue_draw(GTK_WIDGET(l_EQ_curve)); 
          }
        break;


        /*  Popup the right click menu.  */

      case 3:
        gtk_menu_popup (HDEQ_menu, NULL, NULL, NULL, NULL, event->button, gtk_get_current_event_time());
        break;


      default:
        break;
      }
}


/*  This comes from the hdeq drawing area button release callback (actually 
    the event box).  Not as much going on here.  Mostly just resetting
    whatever was done in the motion and button press functions.  */

void hdeq_curve_button_release (GdkEventButton  *event)
{
    int           i;


    switch (event->button)
      {
      case 1:
 
        /*  This is a bit weird.  We're just trying to count releases while
            drawing the EQ curve.  If we're drawing and release the first time
            we'll set to 2 (we just started drawing, EQ_drawing was set to 1
            by the button press callback).  If we're drawing and release for 
            the second time we set to 0 (end drawing section).  */

        if (EQ_drawing == 1)
          {
            EQ_drawing = 2;
          }
        else if (EQ_drawing == 2)
          {
            EQ_drawing = 0;
          }


        /*  Set the graphic EQ sliders based on the hand-drawn curve.  */

        geq_set_sliders (EQ_length, EQ_freq_xinterp, EQ_freq_yinterp);
	//	gtk_widget_queue_draw(GTK_WIDGET(l_EQ_curve));
        EQ_mod = 0;

        break;


        /*  Discard the hand drawn curve if we're drawing something.  */

      case 2:

        if (EQ_drawing)
          {
            EQ_draw_dir = 0;
            EQ_drawing = 0;

            EQ_input_points = 0;

     //       draw_EQ_curve ();
			gtk_widget_queue_draw(GTK_WIDGET(l_EQ_curve)); 
          }
        break;
      }


    /*  Reset all of the notch and crossover drag functions since we released
        a button (can't drag while the button isn't pressed).  */

    xover_active = 0;
    EQ_drag_l2m = 0;
    EQ_drag_m2h = 0;
    for (i = 0 ; i < NOTCHES ; i++) 
      {
        EQ_notch_drag[i] = 0;
        EQ_notch_Q_drag[i] = 0;
      }


    /*  Set the scene warning button because we've (probably) changed 
        something.  */

    set_scene_warning_button ();
}


/*  Handle a button press in the HDEQ popup menu.  */

void hdeq_popup (int action)
{
    int           i;


    switch (action)
      {

        /*  Reset the HDEQ to 0.  */

      case 0:
        reset_hdeq ();


        /*  Just in case we were drawing something.  */

        if (EQ_drawing)
          {
            EQ_draw_dir = 0;
            EQ_drawing = 0;

            EQ_input_points = 0;
          }
        break;


        /*  Release the parametric controls but leave the EQ as is.  */

      case 1:

        /*  Setting the EQ (and state).  */

        for (i = 0 ; i < EQ_length ; i++) EQ_yinterp[i] = EQ_y_notched[i];
        s_set_value_block (EQ_yinterp, S_EQ_GAIN(0), EQ_length);


        for (i = 1 ; i < NOTCHES - 1 ; i++)
          {
            EQ_notch_gain[i] = 0.0;
            EQ_notch_drag[i] = 0;
            EQ_notch_Q_drag[i] = 0;
            EQ_notch_flag[i] = 0;
            EQ_notch_width[i] = 5;
            EQ_notch_index[i] = nearest_x (EQ_notch_default[i]);


            /*  Set the state so that we can save the scene if we need to.  */

            s_set_description (S_NOTCH_GAIN (i), g_strdup_printf ("Reset notch %d", i));
            s_set_value_ns (S_NOTCH_GAIN (i), EQ_notch_gain[i]);
            s_set_value_ns (S_NOTCH_FREQ (i), EQ_notch_default[i]);
            s_set_value_ns (S_NOTCH_FLAG (i), (float) EQ_notch_flag[i]);
            s_set_value_ns (S_NOTCH_Q (i), (float) EQ_notch_width[i]);
          }


        /*  Just in case we were drawing something.  */

        if (EQ_drawing)
          {
            EQ_draw_dir = 0;
            EQ_drawing = 0;

            EQ_input_points = 0;
          }


        set_EQ ();
  //      draw_EQ_curve ();
		gtk_widget_queue_draw(GTK_WIDGET(l_EQ_curve)); 


        /*  Set the scene warning button so that people will know to save it.  */

        set_scene_warning_button ();

        break;


        /*  If we're drawing something "Cancel" will discard it.  If we're not drawing it does nothing.  */

      case 2:

        if (EQ_drawing)
          {
            EQ_draw_dir = 0;
            EQ_drawing = 0;

            EQ_input_points = 0;

   //         draw_EQ_curve ();
			gtk_widget_queue_draw(GTK_WIDGET(l_EQ_curve)); 
          }
        break;
      }
}


/*  Set the label in the hdeq.  */

void hdeq_curve_set_label (char *string)
{
  if (l_EQ_curve_lbl != NULL) gtk_label_set_text (l_EQ_curve_lbl, string);
}


/*  Gets the gain values from the state functions and sets up everything.  
    We're carrying the two unused variable (id, value) so that we can use this
    as a callback that we set up with s_set_callback above.  They're not ever
    used for anything.  */

void set_EQ_curve_values (int id, float value)
{
    int i;


    for (i = 0 ; i < EQ_INTERP ; i++)
      {
        EQ_yinterp[i] = s_get_value (S_EQ_GAIN (0) + i);
      }

    for (i = 0 ; i < NOTCHES ; i++)
      {
        EQ_notch_flag[i] = NINT (s_get_value (S_NOTCH_FLAG (i)));

        EQ_notch_width[i] = NINT (s_get_value (S_NOTCH_Q (i)));
        EQ_notch_index[i] = nearest_x (s_get_value (S_NOTCH_FREQ (i)));
        EQ_notch_gain[i] = s_get_value (S_NOTCH_GAIN (i));
      }


    /*  Replace shelf and notch areas.  */

    insert_notch ();


    /*  Set the GEQ coefs and faders.  */
	if(gui_mode == 0){ // default
	//	set_EQ ();
		EQ_mod = 1;
	} else {
		EQ_mod = 0;
	}
	

    /*  Redraw the curve.  */

 //   draw_EQ_curve ();
	gtk_widget_queue_draw(GTK_WIDGET(l_EQ_curve)); 
}


/*  Reset the crossovers.  */

void hdeq_set_xover ()
{
    process_set_low2mid_xover ((float) pow (10.0, 
                                            s_get_value (S_XOVER_FREQ(0))));
    process_set_mid2high_xover ((float) pow (10.0, 
                                             s_get_value (S_XOVER_FREQ(1))));
    
	printf("%f %f \n", 	s_get_value (S_XOVER_FREQ(0)), s_get_value (S_XOVER_FREQ(1)));
    hdeq_low2mid_init ();
    hdeq_mid2high_init ();
}


/*  Set the lower gain limit for the hdeq and the geq.  */

void hdeq_set_lower_gain (float gain)
{
  EQ_gain_lower = gain;

  l_low2mid_adj = gtk_range_get_adjustment ((GtkRange *) l_low2mid);
  EQ_curve_range_x = gtk_adjustment_get_upper (l_low2mid_adj) - gtk_adjustment_get_lower (l_low2mid_adj);

  EQ_curve_range_y = EQ_gain_upper - EQ_gain_lower;

// draw_EQ_curve ();
  gtk_widget_queue_draw(GTK_WIDGET(l_EQ_curve)); 

  set_scene_warning_button ();
}


/*  Set the upper gain limit for the hdeq and the geq.  */

void hdeq_set_upper_gain (float gain)
{
  EQ_gain_upper = gain;

  l_low2mid_adj = gtk_range_get_adjustment ((GtkRange *) l_low2mid);
  EQ_curve_range_x = gtk_adjustment_get_lower (l_low2mid_adj) - gtk_adjustment_get_lower (l_low2mid_adj);

  EQ_curve_range_y = EQ_gain_upper - EQ_gain_lower;

 // draw_EQ_curve ();
  gtk_widget_queue_draw(GTK_WIDGET(l_EQ_curve)); 

  set_scene_warning_button ();
}


float hdeq_get_lower_gain ()
{
  return (EQ_gain_lower);
}


float hdeq_get_upper_gain ()
{
  return (EQ_gain_upper);
}


/*  Write the annotation for the compressor curves when you move the cursor in
    the curve box.  */

static void comp_write_annotation (int i, char *string)
{
    PangoLayout    *pl;
    PangoRectangle ink_rect;


    /*  Clear the annotation area.  */

    pl = pango_layout_new (comp_pc[i]);  
    pango_layout_set_text (pl, "-99 , -99", -1);
    pango_layout_get_pixel_extents (pl, &ink_rect, NULL);

//    gdk_window_clear_area (comp_drawable[i], 3, 3, ink_rect.width + 5,
//		    ink_rect.height + 5);
//    gdk_gc_set_foreground (comp_gc[i], get_color (TEXT_COLOR));

    pl = pango_layout_new (comp_pc[i]);  
    pango_layout_set_text (pl, string, -1);


 //   gdk_draw_layout (comp_drawable[i], comp_gc[i], 5, 5, pl);
}


void comp_curve_update(int i)
{
	
		gtk_widget_queue_draw(GTK_WIDGET(l_comp_curve[i])); 
	
}

/*  Draw the compressor curve (0-2).  */

void draw_comp_curve (int i, cairo_t *cr)
{
    int              j, x0, y0 = 0.0, x1 = 0.0, y1 = 0.0;
    float            x, y;
    comp_settings    comp;


    if (!comp_realized[i]) return;


    /*  Clear the curve drawing area.  */

//    gdk_window_clear_area (comp_drawable[i], 0, 0, comp_curve_width[i], 
//        comp_curve_height[i]);
//    gdk_gc_set_line_attributes (comp_gc[i], 1, GDK_LINE_SOLID, GDK_CAP_BUTT, 
//        GDK_JOIN_MITER);


    /*  Plot the grid lines.  */

    for (j = NINT (comp_start_x[i]) ; j <= NINT (comp_end_x[i]) ; j++)
      {
        if (!(j % 10))
          {
            x1 = NINT (((float) (j - comp_start_x[i]) / 
                comp_curve_range_x[i]) * comp_curve_width[i]);

//            gdk_draw_line (comp_drawable[i], comp_gc[i], x1, 0, x1, 
//                comp_curve_height[i]);
				cairo_move_to (cr, x1, 0); 
				cairo_line_to (cr, x1, comp_curve_height[i]);
				cairo_stroke (cr);
          }
      }

    for (j = NINT (comp_start_y[i]) ; j <= NINT (comp_end_y[i]) ; j++)
      {
        if (!(j % 10))
          {
            if (!j)
              {
 //               gdk_gc_set_line_attributes (comp_gc[i], 2, GDK_LINE_SOLID, 
 //                   GDK_CAP_BUTT, GDK_JOIN_MITER);
					cairo_set_line_width (cr, 2.0);
              }
            else
              {
 //               gdk_gc_set_line_attributes (comp_gc[i], 1, GDK_LINE_SOLID, 
 //                   GDK_CAP_BUTT, GDK_JOIN_MITER);
					cairo_set_line_width (cr, 1.0);
              }

            y1 = comp_curve_height[i] - NINT (((float) (j - comp_start_y[i]) / 
                comp_curve_range_y[i]) * comp_curve_height[i]);

 //           gdk_draw_line (comp_drawable[i], comp_gc[i], 0, y1, 
 //               comp_curve_width[i], y1);
				cairo_move_to (cr, 0, y1); 
				cairo_line_to (cr, comp_curve_width[i], y1);
				cairo_stroke (cr);
          }
      }


    /*  Plot the curves.  */

//    gdk_gc_set_line_attributes (comp_gc[i], 2, GDK_LINE_SOLID, GDK_CAP_BUTT,
//        GDK_JOIN_MITER);
//    gdk_gc_set_foreground (comp_gc[i], get_color (LOW_BAND_COLOR + i));

	color = get_color (LOW_BAND_COLOR + i);
    cairo_set_source_rgb (cr,color->red,color->green, color->blue);
    
    comp = comp_get_settings (i);

    x0 = 999.0;
    for (x = comp_start_x[i] ; x <= comp_end_x[i] ; x += 0.5f) 
      {
        x1 = NINT (((x - comp_start_x[i]) / comp_curve_range_x[i]) * 
            comp_curve_width[i]);

        y = eval_comp (comp.threshold, comp.ratio, comp.knee, x) +
	    comp.makeup_gain;

        y1 = comp_curve_height[i] - NINT (((y - comp_start_y[i]) / 
            comp_curve_range_y[i]) * comp_curve_height[i]);

        if (x0 != 999.0) {
 //           gdk_draw_line (comp_drawable[i], comp_gc[i], x0, y0, x1, y1);
 
			cairo_move_to (cr, x0, y0); 
			cairo_line_to (cr, x1, y1);
			cairo_stroke (cr);
			
		}	

        x0 = x1;
        y0 = y1;
      }
//    gdk_gc_set_line_attributes (comp_gc[i], 1, GDK_LINE_SOLID, GDK_CAP_BUTT,
//        GDK_JOIN_MITER);
//    gdk_gc_set_foreground (comp_gc[i], get_color (TEXT_COLOR));
}


/*  The compressor curve expose/resize callback (0-2).  */

void comp_curve_draw (GtkWidget *widget, cairo_t *cr, int i)
{
    /*  Since we're doing inclusive plots on the compressor curves we'll
        not decrement the width and height.  */
	GtkAllocation *allocation = g_new0 (GtkAllocation, 1);
	gtk_widget_get_allocation(GTK_WIDGET(widget), allocation);
	
    comp_curve_width[i] = allocation->width;
    comp_curve_height[i] = allocation->height;
	g_free(allocation);	
	
    draw_comp_curve (i, cr);
}


/*  The compressor curve realize callback (0-2).  */

void comp_curve_realize (GtkWidget *widget, int i)
{
//    comp_drawable[i] = widget->window;

    comp_start_x[i] = -60.0;
    comp_end_x[i] = 0.0;
    comp_start_y[i] = -60.0;
    comp_end_y[i] = 30.0;

    comp_curve_range_x[i] = comp_end_x[i] - comp_start_x[i];
    comp_curve_range_y[i] = comp_end_y[i] - comp_start_y[i];

//    comp_gc[i] = widget->style->fg_gc[GTK_WIDGET_STATE (widget)];


    comp_pc[i] = gtk_widget_get_pango_context (widget);


    comp_realized[i] = 1;
}


/*  The compressor curve drawing area motion callback (0-2).  */

void comp_curve_box_motion (int i, GdkEventMotion  *event)
{
    float          x, y;
    char           *coords = NULL;


    x = comp_start_x[i] + (((float) event->x / 
        (float) comp_curve_width[i]) * comp_curve_range_x[i]);


    y = comp_start_y[i] + ((((float) comp_curve_height[i] - (float) event->y) /
        (float) comp_curve_height[i]) * comp_curve_range_y[i]);


    coords = g_strdup_printf ("%d , %d    ", NINT (x), NINT (y));
    comp_write_annotation (i, coords);
    free(coords);
}


/*  Leaving the box/curve, turn off highlights in the labels of the box and 
    curve.  */

void comp_box_leave (int i)
{
    gtk_widget_modify_fg ((GtkWidget *) l_comp_lbl[i], GTK_STATE_NORMAL, 
                          get_color (TEXT_COLOR));
    gtk_widget_modify_fg ((GtkWidget *) l_c_curve_lbl[i], GTK_STATE_NORMAL, 
                          get_color (TEXT_COLOR));
}


/*  Entering the box/curve, turn on highlights in the labels of the box and 
    curve.  */

void comp_box_enter (int i)
{
    gtk_widget_modify_fg ((GtkWidget *) l_comp_lbl[i], GTK_STATE_NORMAL, 
                          get_color (LOW_BAND_COLOR + i));
    gtk_widget_modify_fg ((GtkWidget *) l_c_curve_lbl[i], GTK_STATE_NORMAL, 
                          get_color (LOW_BAND_COLOR + i));
}


/*  Saving the current notebook page on a switch, see callbacks.c.  This saves
    us querying the GUI 10 times per second from spectrum_update.  */

void hdeq_notebook1_set_page (guint page_num)
{
    notebook1_page = page_num;
}


/*  Return the current notebook page - 0 = hdeq, 1 = geq, 2 = spectrum,
    3 = options.  */

int get_current_notebook1_page ()
{
    return (notebook1_page);
}
