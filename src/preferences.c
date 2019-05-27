/*
 *  preferences.c -- Preferences (color, crossfade time) dialog.
 *
 *  Copyright (C) 2004 Jan C. Depner.
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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>


#include "preferences.h"
#include "callbacks.h"
#include "hdeq.h"
#include "geq.h"
#include "main.h"
#include "help.h"
#include "interface.h"
#include "intrim.h"
#include "support.h"
#include "process.h"
#include "compressor-ui.h"
#include "state.h"
#include "spectrum.h"
//#include "gtkmeter.h"


static char color_help[] = {
"    This is a standard color selection dialog.  Push buttons and see what \
happens.  If you don't like the color just press cancel.  When you've got \
the color you want (fuschia, puce, chartreuse, whatever) just press OK.\n"
};


static GtkWidget         *pref_dialog, *color_dialog, *colorsel;
static GdkVisual 		 *visual = NULL; 
//static GdkColormap       *colormap = NULL;
static GdkColor          color[COLORS];
static int               color_id;
static GtkComboBox       *l_limiter_combo, *l_SpectrumComboBox, *l_ColorsComboBox;
static GtkSpinButton     *l_hdeq_lower_gain, *l_hdeq_upper_gain, *l_crossfade_time, 
                         *l_inmeter_warn, *l_spectrum_freq, *l_rms_time_slice, 
                         *l_xo_delay_time_low, *l_xo_delay_time_mid;
static GtkToggleButton   *l_out_meter_peak, *l_out_meter_full, *l_rms_meter_peak, 
                         *l_rms_meter_full, *l_fft, *l_iir; 
static gboolean          initialized = FALSE;


static void color_ok_callback (GtkWidget *w, gpointer user_data);
static void color_cancel_callback (GtkWidget *w, gpointer user_data);
static void color_help_callback (GtkWidget *w, gpointer user_data);


/*  Just like fgets but strips trailing LF/CR.  */

char *ngets (char *s, int size, FILE *stream)
{
  if (fgets (s, size, stream) == NULL) return (NULL);

  while( strlen(s)>0 && (s[strlen(s)-1] == '\n' || s[strlen(s)-1] == '\r') )
    s[strlen(s)-1] = '\0';

  if (s[strlen (s) - 1] == '\n') s[strlen (s) - 1] = 0;


  return (s);
}


/*
  This function opens and reads a file called jamin-defaults.  We're only writing
  out GUI colors so that people can edit the colors in the defaults file
  instead of using the interface to set the colors.  Most preferences are
  stored via the functions in the state.c file.  Yes, Virginia, this should be
  XML but since I don't know XML very well (and I'm lazy), it's not.  If someone
  with more knowledge and energy than me would like to change it, be my guest.
*/

void preferences_init()
{
  char            varin[128], info[128], file[512];
  FILE            *fp = NULL;
  unsigned short  i, j, k;


  pref_dialog = create_pref_dialog ();


  l_SpectrumComboBox = GTK_COMBO_BOX (lookup_widget (pref_dialog, "SpectrumComboBox"));
  gtk_combo_box_set_active (l_SpectrumComboBox, process_get_spec_mode ());


  l_ColorsComboBox = GTK_COMBO_BOX (lookup_widget (pref_dialog, "ColorsComboBox"));
  gtk_combo_box_set_active (l_ColorsComboBox, LOW_BAND_COLOR);


  l_limiter_combo = GTK_COMBO_BOX (lookup_widget (pref_dialog, "limiter_combo"));

  gtk_combo_box_set_active (l_limiter_combo, process_get_limiter_plugin ());

  l_hdeq_lower_gain = GTK_SPIN_BUTTON (lookup_widget (pref_dialog, "MinGainSpin"));
  l_hdeq_upper_gain = GTK_SPIN_BUTTON (lookup_widget (pref_dialog, "MaxGainSpin"));
  l_crossfade_time = GTK_SPIN_BUTTON (lookup_widget (pref_dialog, "CrossfadeTimeSpin"));
  l_inmeter_warn = GTK_SPIN_BUTTON (lookup_widget (pref_dialog, "warningLevelSpinButton"));
  l_spectrum_freq = GTK_SPIN_BUTTON (lookup_widget (pref_dialog, "UpdateFrequencySpin"));
  l_rms_time_slice = GTK_SPIN_BUTTON (lookup_widget (pref_dialog, "rmsTimeValue"));
  l_xo_delay_time_low = GTK_SPIN_BUTTON (lookup_widget (pref_dialog, "LowDelaySpinButton"));
  l_xo_delay_time_mid = GTK_SPIN_BUTTON (lookup_widget (pref_dialog, "MidDelaySpinButton"));

  l_out_meter_peak = GTK_TOGGLE_BUTTON (lookup_widget (pref_dialog, "out_meter_peak_button"));
  l_out_meter_full = GTK_TOGGLE_BUTTON (lookup_widget (pref_dialog, "out_meter_full_button"));
  l_rms_meter_peak = GTK_TOGGLE_BUTTON (lookup_widget (pref_dialog, "rms_meter_peak_button"));
  l_rms_meter_full = GTK_TOGGLE_BUTTON (lookup_widget (pref_dialog, "rms_meter_full_button"));
  l_fft = GTK_TOGGLE_BUTTON (lookup_widget (pref_dialog, "FFTButton"));
  l_iir = GTK_TOGGLE_BUTTON (lookup_widget (pref_dialog, "IIRButton"));


  color_dialog = create_colorselectiondialog1 ();

 /* gtk_widget_get_property(color_dialog,'color-selection',&colorsel);

  GtkWidget *ok_button1;
  GtkWidget *cancel_button1;
  GtkWidget *help_button1;

  g_signal_connect ( gtk_widget_get_property(color_dialog,'ok-button',&ok_button1),
                    "clicked", G_CALLBACK (color_ok_callback), color_dialog);

  g_signal_connect ( gtk_widget_get_property(color_dialog,'cancel-button',&cancel_button1),
                    "clicked", G_CALLBACK (color_cancel_callback), color_dialog);

  g_signal_connect ( gtk_widget_get_property(color_dialog,'help-button',&help_button1),
                    "clicked", G_CALLBACK (color_help_callback), color_dialog);

*/




 // colormap = gdk_colormap_get_system ();
	visual = gdk_visual_get_system ();

  /*  Set all of the colors to the defaults in case someone has edited the
      ~/.jamin/jamin-defaults file and removed (or hosed) one or more of the
      entries.  */

  pref_reset_all_colors ();


  /*  Get user colors.  */

  if (jamin_dir)
    {
      strcpy (file, jamin_dir);
      strcat (file, "jamin-defaults");
    }
  else
    {
      return;
    }

  if ((fp = fopen (file, "r")) != NULL)
    {
      /*  Read each entry.    */
        
      while (ngets (varin, sizeof (varin), fp) != NULL)
        {
          /*  Put everything to the right of the equals sign in 'info'.  */
            
          if (strchr (varin, '=') != NULL)
            strcpy (info, (strchr (varin, '=') + 1));


          /*  Check input for matching strings and load values if
              found.  */
            
          if (strstr (varin, "[LOW BAND COMPRESSOR COLOR]") != NULL)
            {
              sscanf (info, "%hu %hu %hu", &i, &j, &k);
              set_color (&color[LOW_BAND_COLOR], i, j, k);
            }

          if (strstr (varin, "[MID BAND COMPRESSOR COLOR]") != NULL)
            {
              sscanf (info, "%hu %hu %hu", &i, &j, &k);
              set_color (&color[MID_BAND_COLOR], i, j, k);
            }

          if (strstr (varin, "[HIGH BAND COMPRESSOR COLOR]") != NULL)
            {
              sscanf (info, "%hu %hu %hu", &i, &j, &k);
              set_color (&color[HIGH_BAND_COLOR], i, j, k);
            }

          if (strstr (varin, "[GANG HIGHLIGHT COLOR]") != NULL)
            {
              sscanf (info, "%hu %hu %hu", &i, &j, &k);
              set_color (&color[GANG_HIGHLIGHT_COLOR], i, j, k);
            }

          if (strstr (varin, "[PARAMETRIC HANDLE COLOR]") != NULL)
            {
              sscanf (info, "%hu %hu %hu", &i, &j, &k);
              set_color (&color[HANDLE_COLOR], i, j, k);
            }

          if (strstr (varin, "[HDEQ CURVE COLOR]") != NULL)
            {
              sscanf (info, "%hu %hu %hu", &i, &j, &k);
              set_color (&color[HDEQ_CURVE_COLOR], i, j, k);
            }

          if (strstr (varin, "[HDEQ SPECTRUM COLOR]") != NULL)
            {
              sscanf (info, "%hu %hu %hu", &i, &j, &k);
              set_color (&color[HDEQ_SPECTRUM_COLOR], i, j, k);
            }

          if (strstr (varin, "[HDEQ GRID COLOR]") != NULL)
            {
              sscanf (info, "%hu %hu %hu", &i, &j, &k);
              set_color (&color[HDEQ_GRID_COLOR], i, j, k);
            }

          if (strstr (varin, "[HDEQ BACKGROUND COLOR]") != NULL)
            {
              sscanf (info, "%hu %hu %hu", &i, &j, &k);
              set_color (&color[HDEQ_BACKGROUND_COLOR], i, j, k);
            }

          if (strstr (varin, "[TEXT COLOR]") != NULL)
            {
              sscanf (info, "%hu %hu %hu", &i, &j, &k);
              set_color (&color[TEXT_COLOR], i, j, k);
            }

          if (strstr (varin, "[METER NORMAL COLOR]") != NULL)
            {
              sscanf (info, "%hu %hu %hu", &i, &j, &k);
              set_color (&color[METER_NORMAL_COLOR], i, j, k);
            }

          if (strstr (varin, "[METER WARNING COLOR]") != NULL)
            {
              sscanf (info, "%hu %hu %hu", &i, &j, &k);
              set_color (&color[METER_WARNING_COLOR], i, j, k);
            }

          if (strstr (varin, "[METER OVER COLOR]") != NULL)
            {
              sscanf (info, "%hu %hu %hu", &i, &j, &k);
              set_color (&color[METER_OVER_COLOR], i, j, k);
            }

          if (strstr (varin, "[METER PEAK COLOR]") != NULL)
            {
              sscanf (info, "%hu %hu %hu", &i, &j, &k);
              set_color (&color[METER_PEAK_COLOR], i, j, k);
            }
        }


      fclose (fp);
    }

  if (process_limiter_plugins_available () < 2) gtk_widget_set_sensitive (GTK_WIDGET (l_limiter_combo), FALSE);

  initialized = TRUE;
}

void pref_set_all_values ()
{
 // gtk_spin_button_set_value (l_hdeq_lower_gain, hdeq_get_lower_gain ());
 // gtk_spin_button_set_value (l_hdeq_upper_gain, hdeq_get_upper_gain ());
  gtk_spin_button_set_value (l_crossfade_time, s_get_crossfade_time ());
  gtk_spin_button_set_value (l_inmeter_warn, intrim_inmeter_get_warn ());
  gtk_spin_button_set_value (l_spectrum_freq, get_spectrum_freq ());
  gtk_spin_button_set_value (l_rms_time_slice, process_get_rms_time_slice ());
  gtk_spin_button_set_value (l_xo_delay_time_low, process_get_xo_delay_time (XO_LOW));
  gtk_spin_button_set_value (l_xo_delay_time_mid, process_get_xo_delay_time (XO_MID));

  g_signal_handlers_block_by_func (l_out_meter_peak, on_out_meter_peak_button_clicked, NULL);
  g_signal_handlers_block_by_func (l_out_meter_full, on_out_meter_full_button_clicked, NULL);
  g_signal_handlers_block_by_func (l_rms_meter_peak, on_rms_meter_peak_button_clicked, NULL);
  g_signal_handlers_block_by_func (l_rms_meter_full, on_rms_meter_full_button_clicked, NULL);
  g_signal_handlers_block_by_func (l_fft, on_FFTButton_clicked, NULL);
  g_signal_handlers_block_by_func (l_iir, on_IIRButton_clicked, NULL);

  gtk_toggle_button_set_active (l_out_meter_peak, intrim_get_out_meter_peak_pref ());
  gtk_toggle_button_set_active (l_out_meter_full, !intrim_get_out_meter_peak_pref ());
  gtk_toggle_button_set_active (l_rms_meter_peak, intrim_get_rms_meter_peak_pref ());
  gtk_toggle_button_set_active (l_rms_meter_full, !intrim_get_rms_meter_peak_pref ());
  gtk_toggle_button_set_active (l_fft, (process_get_crossover_type () == FFT));
  gtk_toggle_button_set_active (l_iir, (process_get_crossover_type () == IIR));

  g_signal_handlers_unblock_by_func (l_out_meter_peak, on_out_meter_peak_button_clicked, NULL);
  g_signal_handlers_unblock_by_func (l_out_meter_full, on_out_meter_full_button_clicked, NULL);
  g_signal_handlers_unblock_by_func (l_rms_meter_peak, on_rms_meter_peak_button_clicked, NULL);
  g_signal_handlers_unblock_by_func (l_rms_meter_full, on_rms_meter_full_button_clicked, NULL);
  g_signal_handlers_unblock_by_func (l_fft, on_FFTButton_clicked, NULL);
  g_signal_handlers_unblock_by_func (l_iir, on_IIRButton_clicked, NULL);
  
  gtk_combo_box_set_active (l_SpectrumComboBox, process_get_spec_mode ());
  gtk_combo_box_set_active (l_limiter_combo, process_get_limiter_plugin ());
}


GdkColor *get_color (int color_id)
{
  return (&color[color_id]);
}


/*  Generic color setting.  */

void set_color (GdkColor *color, unsigned short red, unsigned short green, 
                unsigned short blue)
{
  color->red = red;
  color->green = green;
  color->blue = blue;

 // gdk_colormap_alloc_color (colormap, color, FALSE, TRUE);
//  gtk_widget_modify_bg(visual, , &color);
}


/*  Pop up the EQ options dialog.  */

void popup_pref_dialog (int updown)
{
  /*  Pop up on 1.  */

  if (updown)
    {
      gtk_widget_show (pref_dialog);
    }


  /*  Pop down on 0.  */

  else
    {
      gtk_widget_hide (pref_dialog);
    }
}


/*  Pop up the color dialog.  */

void popup_color_dialog (int id)
{
  GdkColor *ptr;


  /*  We don't want to do this until after the colors combo box has been set the first time.  */

  if (initialized)
    {
      color_id = id;

      ptr = &color[id];


      gtk_color_selection_set_current_color ((GtkColorSelection *) colorsel, ptr);


      gtk_widget_show (color_dialog);
    }
}


void pref_force_color_change ()
{
  static GdkRectangle rect;


  repaint_gang_labels ();
 // draw_EQ_curve ();


  /*  Force all compressor curves.  */

 // draw_comp_curve (0);
 // draw_comp_curve (1);
 // draw_comp_curve (2);


  /*  Force all meter redraws.  */

 // gtk_meter_set_color (METER_NORMAL_COLOR);
 // gtk_meter_set_color (METER_WARNING_COLOR);
 // gtk_meter_set_color (METER_OVER_COLOR);
 // gtk_meter_set_color (METER_PEAK_COLOR);


  /*  Force an expose to change the text color.  */
  GtkAllocation *allocation = g_new0 (GtkAllocation, 1);
  gtk_widget_get_allocation(GTK_WIDGET(main_window), allocation);
  
  rect.x = allocation->x;
  rect.y = allocation->y;
  rect.width = allocation->width;
  rect.height = allocation->height;
  
  g_free (allocation);
  
  gdk_window_invalidate_rect (gtk_widget_get_window(main_window), &rect, TRUE);
  gdk_window_process_updates (gtk_widget_get_window(main_window), TRUE);
}


static void color_ok_callback (GtkWidget *w, gpointer user_data)
{
  GdkColor l_color;


  gtk_color_selection_get_current_color ((GtkColorSelection *) colorsel, 
                                         &l_color);

  set_color (&color[color_id], l_color.red, l_color.green, l_color.blue);

  pref_force_color_change ();

  gtk_widget_hide (color_dialog);
}


static void color_cancel_callback (GtkWidget *w, gpointer user_data)
{
  gtk_widget_hide (color_dialog);
}


static void color_help_callback (GtkWidget *w, gpointer user_data)
{
  message (GTK_MESSAGE_INFO, color_help);
}


/*  We're only writing out GUI colors so that people can edit the colors in the defaults file
    instead of using the interface to set the colors.  Most preferences are stored via the 
    functions in the state.c file.  */

void pref_write_jamin_defaults ()
{
  char     file[512];
  FILE     *fp = NULL;


  if (jamin_dir)
    {
      strcpy (file, jamin_dir);
      strcat (file, "jamin-defaults");
    }
  else
    {
      return;
    }
     
  if ((fp = fopen (file, "w")) != NULL)
    {
      fprintf (fp, "JAMin defaults file V%s\n",  VERSION);

      fprintf (fp, "[LOW BAND COMPRESSOR COLOR]  = %hu %hu %hu\n",
               color[LOW_BAND_COLOR].red, 
               color[LOW_BAND_COLOR].green,
               color[LOW_BAND_COLOR].blue);

      fprintf (fp, "[MID BAND COMPRESSOR COLOR]  = %hu %hu %hu\n",
               color[MID_BAND_COLOR].red, 
               color[MID_BAND_COLOR].green,
               color[MID_BAND_COLOR].blue);

      fprintf (fp, "[HIGH BAND COMPRESSOR COLOR] = %hu %hu %hu\n",
               color[HIGH_BAND_COLOR].red, 
               color[HIGH_BAND_COLOR].green,
               color[HIGH_BAND_COLOR].blue);

      fprintf (fp, "[GANG HIGHLIGHT COLOR]       = %hu %hu %hu\n",
               color[GANG_HIGHLIGHT_COLOR].red, 
               color[GANG_HIGHLIGHT_COLOR].green,
               color[GANG_HIGHLIGHT_COLOR].blue);

      fprintf (fp, "[PARAMETRIC HANDLE COLOR]    = %hu %hu %hu\n",
               color[HANDLE_COLOR].red, 
               color[HANDLE_COLOR].green,
               color[HANDLE_COLOR].blue);

      fprintf (fp, "[HDEQ CURVE COLOR]           = %hu %hu %hu\n",
               color[HDEQ_CURVE_COLOR].red, 
               color[HDEQ_CURVE_COLOR].green,
               color[HDEQ_CURVE_COLOR].blue);

      fprintf (fp, "[HDEQ SPECTRUM COLOR]        = %hu %hu %hu\n",
               color[HDEQ_SPECTRUM_COLOR].red, 
               color[HDEQ_SPECTRUM_COLOR].green,
               color[HDEQ_SPECTRUM_COLOR].blue);

      fprintf (fp, "[HDEQ GRID COLOR]            = %hu %hu %hu\n",
               color[HDEQ_GRID_COLOR].red, 
               color[HDEQ_GRID_COLOR].green,
               color[HDEQ_GRID_COLOR].blue);

      fprintf (fp, "[HDEQ BACKGROUND COLOR]      = %hu %hu %hu\n",
               color[HDEQ_BACKGROUND_COLOR].red, 
               color[HDEQ_BACKGROUND_COLOR].green,
               color[HDEQ_BACKGROUND_COLOR].blue);

      fprintf (fp, "[TEXT COLOR]                 = %hu %hu %hu\n",
               color[TEXT_COLOR].red, 
               color[TEXT_COLOR].green,
               color[TEXT_COLOR].blue);

      fprintf (fp, "[METER NORMAL COLOR]         = %hu %hu %hu\n",
               color[METER_NORMAL_COLOR].red, 
               color[METER_NORMAL_COLOR].green,
               color[METER_NORMAL_COLOR].blue);

      fprintf (fp, "[METER WARNING COLOR]        = %hu %hu %hu\n",
               color[METER_WARNING_COLOR].red, 
               color[METER_WARNING_COLOR].green,
               color[METER_WARNING_COLOR].blue);

      fprintf (fp, "[METER OVER COLOR]           = %hu %hu %hu\n",
               color[METER_OVER_COLOR].red, 
               color[METER_OVER_COLOR].green,
               color[METER_OVER_COLOR].blue);

      fprintf (fp, "[METER PEAK COLOR]           = %hu %hu %hu\n",
               color[METER_PEAK_COLOR].red, 
               color[METER_PEAK_COLOR].green,
               color[METER_PEAK_COLOR].blue);

      fclose (fp);
    }
}


void pref_reset_all_colors ()
{
  set_color (&color[LOW_BAND_COLOR], 0, 50000, 0);
  set_color (&color[MID_BAND_COLOR], 0, 0, 60000);
  set_color (&color[HIGH_BAND_COLOR], 60000, 0, 0);
  set_color (&color[GANG_HIGHLIGHT_COLOR], 65535, 0, 0);
  set_color (&color[HANDLE_COLOR], 65535, 65535, 0);
  set_color (&color[HDEQ_CURVE_COLOR], 65535, 65535, 65535);
  set_color (&color[HDEQ_SPECTRUM_COLOR], 0, 65535, 65535);
  set_color (&color[HDEQ_GRID_COLOR], 0, 36611, 0);
  set_color (&color[HDEQ_BACKGROUND_COLOR], 0, 0, 0);
  set_color (&color[TEXT_COLOR], 0, 0, 0);
  set_color (&color[METER_NORMAL_COLOR], 0, 60000, 0);
  set_color (&color[METER_WARNING_COLOR], 50000, 55000, 0);
  set_color (&color[METER_OVER_COLOR], 60000, 0, 0);
  set_color (&color[METER_PEAK_COLOR], 0, 0, 60000);
}
