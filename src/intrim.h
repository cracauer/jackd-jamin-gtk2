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
 *  $Id: intrim.h,v 1.9 2007/05/12 16:28:35 jdepner Exp $
 */

#ifndef INTRIM_H
#define INTRIM_H

void bind_intrim();
void in_meter_value(float amp[]);
void out_meter_value(float amp[]);
void rms_meter_value(float amp[]);
void intrim_set_out_meter_peak_pref (gboolean pref);
gboolean intrim_get_out_meter_peak_pref ();
void intrim_set_rms_meter_peak_pref (gboolean pref);
gboolean intrim_get_rms_meter_peak_pref ();
void update_pan_label(float balance);
void intrim_inmeter_reset_peak();
void intrim_outmeter_reset_peak();
void intrim_rmsmeter_reset_peak ();
void intrim_inmeter_set_warn (float level);
void intrim_outmeter_set_warn (float level);
void intrim_rmsmeter_set_warn (float level);
float intrim_inmeter_get_warn ();
float intrim_outmeter_get_warn ();
float intrim_rmsmeter_get_warn ();

extern float in_gain[], out_gain;
extern float in_trim_gain;
extern float in_pan_gain[];

#endif
