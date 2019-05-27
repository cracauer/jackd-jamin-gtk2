/*
 *  Copyright (C) 2003 Jan C. Depner, Jack O'Quin
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
 *  $Id: io-menu.h,v 1.6 2004/04/08 15:37:37 joq Exp $
 */

#ifndef IO_MENU_H
#define IO_MENU_H

/* Initialization. */
void iomenu_bind(GtkWidget *main_window);

/* Ports menu item callback. */
void iomenu_pull_down_ports();

#endif
