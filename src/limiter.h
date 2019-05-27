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
 *  $Id: limiter.h,v 1.9 2007/06/22 01:25:03 jdepner Exp $
 */

#ifndef LIMITER_H
#define LIMITER_H

#include <process.h>

#define LIM_INGAIN        0
#define LIM_LIMIT         1
#define LIM_RELEASE       2
#define LIM_ATTENUATION   3
#define LIM_IN_1          4
#define LIM_IN_2          5
#define LIM_OUT_1         6
#define LIM_OUT_2         7
#define LIM_LATENCY       8
#define LIM_LOGSCALE      9

typedef struct {
	float ingain;
	float limit;
	float release;
	float attenuation;
	float latency;
        float logscale;
	LADSPA_Handle handle;
} lim_settings;

static inline void lim_connect(plugin *p, lim_settings *s, float *left, float
		*right) {
	plugin_connect_port(p, s->handle, LIM_INGAIN, &(s->ingain));
	plugin_connect_port(p, s->handle, LIM_LIMIT, &(s->limit));
	plugin_connect_port(p, s->handle, LIM_RELEASE, &(s->release));
	plugin_connect_port(p, s->handle, LIM_ATTENUATION, &(s->attenuation));
	plugin_connect_port(p, s->handle, LIM_IN_1, left);
	plugin_connect_port(p, s->handle, LIM_IN_2, right);
	plugin_connect_port(p, s->handle, LIM_OUT_1, left);
	plugin_connect_port(p, s->handle, LIM_OUT_2, right);
	plugin_connect_port(p, s->handle, LIM_LATENCY, &(s->latency));
        plugin_connect_port(p, s->handle, LIM_LOGSCALE, &(s->logscale));

	/* Make sure that it is set to something */
	s->ingain = 0.0f;
	s->limit = 0.0f;
	s->release = 0.01f;
	s->attenuation = 0.0f;
        s->logscale = 0.75f;
}

#endif
