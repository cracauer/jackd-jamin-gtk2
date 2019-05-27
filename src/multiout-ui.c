/*
 *  Copyright (C) 2013 Patrick Shirkey
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
 *  $Id: multiout-ui.c,v 1.1 2013/02/09 15:47:30 kotau Exp $
 */

#include <stdio.h>
#include <gtk/gtk.h>
#include <math.h>
//#include <clutter/clutter.h>
//#include <clutter-gtk/clutter-gtk.h>


#include "process.h"
#include "support.h"
#include "main.h"
#include "multiout-ui.h"
//#include "gtkmeter.h"
#include "state.h"
#include "db.h"


//extern int global_gui; 

//ClutterActor *stage = NULL;

void multiout_ui_build (GtkWidget *window)
{

  GtkWidget *hbox = GTK_BUTTON(lookup_widget(multiout_window, "hbox_w4_1"));
 // ClutterColor stage_color = { 0x00, 0x00, 0x00, 0xff }; /* Black */

 // gtk_clutter_init (NULL, NULL);


  /* Create the clutter widget: */
 // GtkWidget *clutter_widget = gtk_clutter_embed_new ();
 // gtk_box_pack_start (GTK_BOX (hbox), clutter_widget, TRUE, TRUE, 0);
 // gtk_widget_show (clutter_widget);

  /* Set the size of the widget, 
   * because we should not set the size of its stage when using GtkClutterEmbed.
   */ 
  //gtk_widget_set_size_request (clutter_widget, 200, 200);

  /* Get the stage and set its size and color: */
 // stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (clutter_widget));
 // clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);

  /* Show the stage: */
 // clutter_actor_show (stage);	

}

/* vi:set ts=8 sts=4 sw=4: */
