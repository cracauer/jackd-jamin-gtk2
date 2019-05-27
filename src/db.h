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
 *  $Id: db.h,v 1.2 2003/11/19 15:28:17 theno23 Exp $
 */

#ifndef DB_H
#define DB_H

#define db2lin(g) (powf(10.0f, (g) * 0.05f))

#define lin2db(v) (20.0f * log10f(v))

#endif
