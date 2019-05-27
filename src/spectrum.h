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
 *  $Id: spectrum.h,v 1.6 2004/05/06 10:28:00 jdepner Exp $
 */

#ifndef SPECTRUM_H
#define SPECTRUM_H

void bind_spectrum();
void set_spectrum_freq (int freq);
int get_spectrum_freq ();
void spectrum_timeout_check();

#endif
