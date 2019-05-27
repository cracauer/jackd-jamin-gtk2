/*
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
 *  $Id: transport.h,v 1.6 2003/12/17 04:13:36 joq Exp $
 */

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <jack/transport.h>

void transport_control(jack_nframes_t nframes);
jack_transport_state_t transport_get_state();
double transport_get_time();

/* UI functions */
void transport_play();
void transport_skip(double skip);
void transport_set_time(double time);
void transport_stop();
void transport_toggle_play();

#endif /* TRANSPORT_H */
