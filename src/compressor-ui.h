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
 *  $Id: compressor-ui.h,v 1.9 2004/05/06 09:42:59 theno23 Exp $
 */

#ifndef COMPRESSOR_UI_H
#define COMPRESSOR_UI_H

#define XO_BANDS 3

void bind_compressors();
void compressor_meters_update();
void comp_set_auto(int band, int state);
void repaint_gang_labels ();
void comp_gang_at (int band);
void comp_gang_re (int band);
void comp_gang_th (int band);
void comp_gang_ra (int band);
void comp_gang_kn (int band);
void comp_gang_ma (int band);
gboolean comp_at_ganged(int band);
gboolean comp_re_ganged(int band);
gboolean comp_th_ganged(int band);
gboolean comp_ra_ganged(int band);
gboolean comp_kn_ganged(int band);
gboolean comp_ma_ganged(int band);
void suspend_ganging ();
gboolean unsuspend_ganging (gpointer data);

#endif
