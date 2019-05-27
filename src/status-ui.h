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
 *  $Id: status-ui.h,v 1.7 2003/11/19 15:28:17 theno23 Exp $
 */

#ifndef STATUS_UI_H
#define STATUS_UI_H

/* JACK status display UI functions */

void status_update(GtkWidget *main_window);
void status_set_time(GtkWidget *main_window);

#endif /* STATUS_UI_H */
