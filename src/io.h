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
 *  $Id: io.h,v 1.15 2013/02/06 03:42:39 kotau Exp $
 */

#ifndef IO_H
#define IO_H

#include <jack/types.h>

/* types of latency sources */
#define LAT_BUFFERS	0		/* I/O buffering */
#define LAT_FFT		1		/* Fourier transform */
#define LAT_LIMITER	2		/* Limiter */
#define LAT_NSOURCES	3

extern jack_client_t *client;		/* JACK client structure */
extern char *client_name;		/* JACK client name */
extern int nchannels;			/* actual number of channels */
extern int bchannels;		/* actual number of output channels */
extern jack_port_t *input_ports[];
extern jack_port_t *output_ports[];

void io_activate();
void io_cleanup();
void io_init(int argc, char *argv[]);
void io_set_latency(int latency_source, jack_nframes_t delay);

#endif
