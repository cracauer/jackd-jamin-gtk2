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
 *  $Id: limiter-ui.h,v 1.5 2007/07/01 15:33:18 jdepner Exp $
 */

#ifndef LIMITER_UI_H
#define LIMITER_UI_H

void bind_limiter();
void limiter_meters_update();
void limiter_inmeter_reset_peak ();
void limiter_outmeter_reset_peak ();
void limiter_set_label (int limiter_plugin);
void limiter_logscale_set_state (gboolean state);

#endif
