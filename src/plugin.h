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
 *  $Id: plugin.h,v 1.2 2003/11/19 15:28:17 theno23 Exp $
 */

#ifndef PLUGIN_H
#define PLUGIN_H

#include "ladspa.h"

typedef struct {
	void *dl;
	LADSPA_Descriptor *descriptor;
} plugin;

void plugin_init();

plugin *plugin_load(char *file);

LADSPA_Handle plugin_instantiate(plugin *p, unsigned long fs);

static inline void plugin_connect_port(plugin *p, LADSPA_Handle h, unsigned
		long port, LADSPA_Data *data);

static inline void plugin_run(plugin *p, LADSPA_Handle *h, unsigned long
		sample_count);

static inline void plugin_connect_port(plugin *p, LADSPA_Handle h, unsigned
		long port, LADSPA_Data *data)
{
	(*p->descriptor->connect_port)(h, port, data);
}

static inline void plugin_run(plugin *p, LADSPA_Handle *h, unsigned long
		sample_count)
{
	(*p->descriptor->run)(h, sample_count);
}

#endif
