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
 *  $Id: geq.h,v 1.11 2003/11/19 15:43:38 theno23 Exp $
 */

#ifndef GEQ_H
#define GEQ_H

#define EQ_BANDS 30

#include <gtk/gtk.h>

void bind_geq();
void geq_set_coefs (int length, float x[], float y[]);
void geq_set_sliders(int length, float x[], float y[]);
void geq_set_range(double min, double max);
void geq_get_freqs_and_gains(float *freqs, float *gains);

GtkAdjustment *geq_get_adjustment(int band);

#endif
