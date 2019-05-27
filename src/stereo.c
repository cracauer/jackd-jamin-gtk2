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
 *  $Id: stereo.c,v 1.7 2004/05/01 13:07:04 jdepner Exp $
 */

#include <stdio.h>

#include "process.h"
#include "support.h"
#include "state.h"
#include "main.h"

void stereo_cb(int id, float value);


void bind_stereo()
{
    int i;


    for (i = 0; i < 3; i++) {
	s_set_callback(S_STEREO_WIDTH(i), stereo_cb);
	process_set_stereo_width(i, 0.0f);


        /*  This is a holdover from when we had per band balance.  It is still
            needed until Steve gets a chance to clean up the stereo balance hacks
            that I've done.  JCD  */

	process_set_stereo_balance(i, 0.0f);
    }
}

void stereo_cb(int id, float value)
{
    int band = id - S_STEREO_WIDTH(0);

    process_set_stereo_width(band, value);
}
