/*
 *  Copyright (C) 2006 Steve Harris
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
 *  $Id: rms.c,v 1.3 2007/06/29 17:17:59 jdepner Exp $
 */

#include <math.h>
#include <stdlib.h>

#include "rms.h"

struct _rms {
    double rm;
    double coef;
};

rms *rms_new(float fs, float time)
{
    rms *r = calloc(1, sizeof(rms));

    r->rm = 0.0001;
    r->coef = 0.5 * (1.0 - exp(-1.0 / (fs * time)));

    return r;
}

float rms_run(rms *r, float x)
{
    x = fabs(x) + 0.0001;
    r->rm += r->coef * ((x * x / r->rm) - r->rm);

    return r->rm;
}

float rms_run_buffer(rms *r, float *x, int length)
{
    int i;

    for (i=0; i<length; i++) {
        rms_run(r, x[i]);
    }

    return r->rm;
}

void rms_free(rms *r)
{
    if (r) free(r);
}

/* vi:set expandtab sts=4 sw=4: */
