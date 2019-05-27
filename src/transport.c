/*
 *  transport.c -- JACK transport control functions.
 *
 *  Copyright (C) 2003 Jack O'Quin
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

#define _ISOC99_SOURCE 1
#include <math.h>
#include <jack/jack.h>

#include "debug.h"
#include "io.h"
#include "transport.h"


/******************* user interface functions *******************/

/* These functions are all called from a GUI thread.  So, their
 * interaction with the DSP engine and its threads needs to be
 * thread-safe.  With the new JACK transport interface, this is not a
 * problem.
 */


jack_transport_state_t transport_get_state()
{
    if (client)
	return jack_transport_query(client, NULL);
    else
	return JackTransportStopped;
}


double transport_get_time()
{
    jack_position_t pos;

    if (client) {
	jack_transport_query(client, &pos);
	return pos.frame / (double) pos.frame_rate;
    } else {
	return 0.0;
    }
}


void transport_play()
{
    if (client)
	jack_transport_start(client);
}


void transport_skip(double skip)
{
    jack_position_t pos;
    double now, then, frate;

    if (client) {
	jack_transport_query(client, &pos);
	frate = pos.frame_rate;
	now = pos.frame / frate;
	then = fmax(nearbyint(now + skip), 0.0);
	jack_transport_locate(client, then * frate);
    }
}


void transport_set_time(double time)
{
    jack_position_t pos;

    if (client) {
	jack_transport_query(client, &pos);
	jack_transport_locate(client, time * pos.frame_rate);
    }
}


void transport_stop()
{
    if (client)
	jack_transport_stop(client);
}


void transport_toggle_play()
{
    if (transport_get_state() == JackTransportStopped)
	transport_play();
    else
	transport_stop();
}
