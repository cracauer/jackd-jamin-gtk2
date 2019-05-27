/*
 *  help.c -- Message dialogs for the JAMin (JACK Audio Mastering 
 *            interface) program.
 *
 *  Copyright (C) 2003 Jan C. Depner.
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>

#include "main.h"
#include "help.h"


static GtkWidget *message_dialog = NULL;


/*  Message types - GTK_MESSAGE_INFO, GTK_MESSAGE_WARNING, GTK_MESSAGE_ERROR */

void message (GtkMessageType type, char *string)
{
    /*  If the dialog is already up, kill it.  */

    if (message_dialog != NULL) 
      gtk_widget_destroy ((GtkWidget *) message_dialog);

    message_dialog = gtk_message_dialog_new ((GtkWindow *) main_window,
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          type, GTK_BUTTONS_CLOSE,
                                          "%s", string);

    g_signal_connect_swapped (message_dialog, "response",
                           G_CALLBACK (gtk_widget_destroy),
                           &message_dialog);


    /*  This little callback goody is what sets message_dialog to NULL when it
        is destroyed.  This makes it so that if we launch a bunch of error
        messages it will destroy any that are already up.  We'll only get the
        last message.  */

    g_signal_connect (message_dialog, "destroy",
                      G_CALLBACK (gtk_widget_destroyed),
                      &message_dialog);

    gtk_widget_show (message_dialog);
}
