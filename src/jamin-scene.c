/*
 *  Copyright (C) 2004 Steve Harris
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
 *  $Id: jamin-scene.c,v 1.6 2006/02/23 03:50:23 jdepner Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <lo/lo.h>
#include "constants.h"

int main(int argc, char *argv[])
{
    int scene;
    lo_address addr;

    if (argc != 2) {
	fprintf(stderr, "Usage: %s <scene-num>\n", argv[0]);

	return 1;
    }

    scene = atoi(argv[1]);
    if (scene < 1 || scene > NUM_SCENES) {
	fprintf(stderr, "Scene must be between 1 and %d\n", NUM_SCENES);

	return 1;
    }

    addr = lo_address_new(NULL, OSC_PORT);
    lo_send(addr, OSC_PATH, "i", scene);

    return 0;
}

/* vi:set ts=8 sts=4 sw=4: */
