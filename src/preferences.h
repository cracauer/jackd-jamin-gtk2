/*
 *  Copyright (C) 2003 Jan C. Depner
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
 *  $Id: preferences.h,v 1.11 2008/02/03 20:43:20 esaracco Exp $
 */

#ifndef PREFERENCES_H
#define PREFERENCES_H


/*  Important note - definition of colors is in the same order as
    the combo box buttons.  Don't add to or rearrange the colors unless
    you set the combo box entries to match.  */

#define LOW_BAND_COLOR        0
#define MID_BAND_COLOR        1
#define HIGH_BAND_COLOR       2
#define GANG_HIGHLIGHT_COLOR  3
#define HANDLE_COLOR          4
#define HDEQ_CURVE_COLOR      5
#define HDEQ_SPECTRUM_COLOR   6
#define HDEQ_GRID_COLOR       7
#define HDEQ_BACKGROUND_COLOR 8
#define TEXT_COLOR            9
#define METER_NORMAL_COLOR    10
#define METER_WARNING_COLOR   11
#define METER_OVER_COLOR      12
#define METER_PEAK_COLOR      13

#define COLORS                14


void preferences_init();
GdkColor *get_color (int color_id);
void set_color (GdkColor *color, unsigned short red, unsigned short green, 
                unsigned short blue);
void popup_pref_dialog (int updown);
void popup_color_dialog (int id);
void pref_force_color_change ();
void pref_write_jamin_defaults ();
void pref_reset_all_colors ();
void pref_set_all_values ();

#endif
