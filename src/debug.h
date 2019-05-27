/*
 *  Copyright (C) 2003 Jan C. Depner, Jack O'Quin, Steve Harris
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
 *  $Id: debug.h,v 1.3 2003/11/19 15:28:17 theno23 Exp $
 */


#ifndef DEBUG_H
#define DEBUG_H

extern int debug_level;			/* current debugging level... */
#define DBG_OFF		0
#define DBG_TERSE	1
#define DBG_NORMAL	2
#define DBG_VERBOSE	3

#define IF_DEBUG(lvl,statements) if (debug_level>=(lvl)) statements;

#endif
