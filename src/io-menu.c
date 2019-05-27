/*
 *  iomenu.c -- GTK-2 interface for managing JACK port connections.
 *
 *  This is intended as a general, reusable interface for applications
 *  using GTK-2 and JACK.  
 *
 *  The iomenu_bind() initialization should be called before entering
 *  gtk_main().
 *
 *  If using glade-2, define a menubar item named "jack_ports"
 *  and call iomenu_pull_down_ports() from its "activate" signal handler.
 */

/*
 *  Copyright (C) 2003 Patrick Shirkey, Jack O'Quin
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
 *  $Id: io-menu.c,v 1.25 2004/04/19 04:43:39 joq Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <jack/jack.h>

#include "support.h"

/* Shared global JACK connection information.  The `client' pointer
 * must be NULL when the JACK connection is inactive.  The
 * `input_ports' and `output_ports' are NULL-terminated arrays of
 * pointers to this client's input and output ports. */
#include "io.h"

/* Global GTK data. */
static GtkWidget   *menubar_item;	/* "Ports" menubar item */
static GtkWidget   *pulldown_menu;	/* "Ports" pulldown menu */
static GtkWidget   *in_item;		/* "Connect" menubar item */
static GtkWidget   *out_item;		/* "Disconnect" menubar item */
static GtkWidget   *iports_menu = NULL;	/* In ports menu */
static GtkWidget   *oports_menu = NULL;	/* Out ports menu */

/* These lists of currently registered input and output ports are
 * updated each time the menu is used. */
static const char **inports = NULL;
static const char **outports = NULL;

/* NULL-terminated lists of client names from inports and outports,
 * each name ending in a ':'. */
#define MAXGROUPS 32
static const char *ingroups[MAXGROUPS]; 
static const char *outgroups[MAXGROUPS];
static size_t ngroup_names = 0;
static char *group_names;		/* group name buffer */

#ifndef HAVE_JACK_CLIENT_NAME_SIZE	/* if earlier version of JACK */
#define jack_client_name_size()		(32 + 1)
#endif

/* * * * * * * * * * * *   Callbacks   * * * * * * * * * * * * * * */

/* connect/disconnect function pointer prototype */
typedef void (*iomenu_callback)(GtkWidget *widget, char *port_name);
typedef int  (*iomenu_check)(const char *group);


/* dislay error message.  This should display a pop-up. */
static void
iomenu_error(char *fmt, ...)
{
    va_list ap;
    char buffer[300];

    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    fprintf(stderr, _("iomenu error: %s\n"), buffer);
}

static jack_port_t  *selected_port;	/* currently selected port */

/* remember the local port selected */
static void
iomenu_select_port(GtkWidget *menu_item, jack_port_t *port)
{
    selected_port = port;
}

/* make connection */
static void
iomenu_connect(GtkWidget *widget, char *port_name)
{
    const char *selected_name;

    if (client == NULL)
	    return;
    selected_name = jack_port_name(selected_port);

    if (jack_port_flags(selected_port) & JackPortIsInput) {
	fprintf(stderr, _("connecting port %s to %s\n"),
		port_name, selected_name);
	if (jack_connect(client, port_name, selected_name)) {
	    iomenu_error(_("unable to connect from %s\n"), port_name);
	}

    } else if (jack_port_flags(selected_port) & JackPortIsOutput) {
	fprintf(stderr, _("connecting port %s to %s\n"),
		selected_name, port_name);
	if (jack_connect(client, selected_name, port_name)) {
	    iomenu_error(_("unable to connect to %s\n"), port_name);
	}
    }
}

/* break connection */
static void
iomenu_disconnect(GtkWidget *widget, char *port_name)
{
    const char *selected_name;

    if (client == NULL)
	    return;
    selected_name = jack_port_name(selected_port);

    if (jack_port_flags(selected_port) & JackPortIsInput) {
	fprintf(stderr, _("disconnecting port %s from %s\n"),
		port_name, selected_name);
	if (jack_disconnect(client, port_name, selected_name)) {
	    iomenu_error(_("unable to disconnect port %s\n"), port_name);
	}

    } else if (jack_port_flags(selected_port) & JackPortIsOutput) {
	fprintf(stderr, _("disconnecting port %s from %s\n"),
		selected_name, port_name);
	if (jack_disconnect(client, selected_name, port_name)) {
	    iomenu_error(_("unable to disconnect port %s\n"), port_name);
	}
    }
}

/* connect (or disconnect) all input ports */
static void
iomenu_all_inputs(GtkWidget *widget, char *group)
{
    int i;
    const char **gports;

    if (client == NULL)
	    return;

    gports = jack_get_ports(client, group, JACK_DEFAULT_AUDIO_TYPE,
			    JackPortIsOutput);

    if (gports == NULL)
	return;

    /* connect or disconnect as many input ports as possible */
    for (i = 0; input_ports[i] && gports[i]; ++i) {

	const char *local_name = jack_port_name(input_ports[i]);

	if (jack_port_connected_to(input_ports[i], gports[i])) {

	    fprintf(stderr, _("disconnecting port %s from %s\n"),
		    gports[i], local_name);

	    if (jack_disconnect(client, gports[i], local_name)) {
		iomenu_error(_("unable to disconnect %s from %s\n"),
			     gports[i], local_name);
	    }

	} else {

	    fprintf(stderr, _("connecting port %s to %s\n"),
		    gports[i], local_name);

	    if (jack_connect(client, gports[i], local_name)) {
		iomenu_error(_("unable to connect %s to %s\n"),
			     gports[i], local_name);
	    }
	}
    }

    free(gports);
}

/* connect (or disconnect) all output ports */
static void
iomenu_all_outputs(GtkWidget *widget, char *group)
{
    int i;
    const char **gports;

    if (client == NULL)
	    return;

    gports = jack_get_ports(client, group, JACK_DEFAULT_AUDIO_TYPE,
			    JackPortIsInput);

    if (gports == NULL)
	return;

    /* connect or disconnect as many output ports as possible */
    for (i = 0; output_ports[i] && gports[i]; ++i) {

	const char *local_name = jack_port_name(output_ports[i]);

	if (jack_port_connected_to(output_ports[i], gports[i])) {

	    fprintf(stderr, _("disconnecting port %s from %s\n"),
		    gports[i], local_name);

	    if (jack_disconnect(client,  local_name, gports[i])) {
		iomenu_error(_("unable to disconnect %s from %s\n"),
			     local_name, gports[i]);
	    }

	} else {

	    fprintf(stderr, _("connecting port %s to %s\n"),
		    gports[i], local_name);

	    if (jack_connect(client, local_name, gports[i])) {
		iomenu_error(_("unable to connect %s to %s\n"),
			     local_name, gports[i]);
	    }
	}
    }

    free(gports);
}


/* * * * * * * * * * * *   Menu Creation   * * * * * * * * * * * * */

GtkWidget *
iomenu_add_item(GtkWidget *menu, GtkWidget *item)
{
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);
    return item;
}

GtkWidget *
iomenu_add_submenu(GtkWidget *item)
{
    GtkWidget *menu = gtk_menu_new();

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
    return menu;
}

GtkWidget *
iomenu_connection_item(jack_port_t *port, const char *connection_name)
{
    iomenu_callback handler;
    GtkWidget *item = gtk_check_menu_item_new_with_label(connection_name);

    if (jack_port_connected_to(port, connection_name)) {
	handler = iomenu_disconnect;	/* disconnect, if selected */
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
    } else {
	handler = iomenu_connect;	/* connect, if selected */
    }

    /* iomenu_select_port() must run before the other callback */
    g_signal_connect(G_OBJECT(item), "toggled",
		     G_CALLBACK(iomenu_select_port), port);
    g_signal_connect(G_OBJECT(item), "toggled",
		     G_CALLBACK(handler), (char *) connection_name);
    
    return item;
}

/* return TRUE if any input port is connected to this group */
static int
iomenu_check_input(const char *group)
{
    const char **gports;

    if (client) {
	gports = jack_get_ports(client, group, JACK_DEFAULT_AUDIO_TYPE,
				JackPortIsOutput);
	if (gports) {
	    int i, j;
	    for (i = 0; input_ports[i]; ++i) {
		for (j = 0; gports[j]; ++j) {
		    if (jack_port_connected_to(input_ports[i],
					       gports[j])) {
			free(gports);
			return TRUE;
		    }
		}
	    }
	    free(gports);
	}
    }
    return FALSE;
}

/* return TRUE if any output port is connected to this group */
static int
iomenu_check_output(const char *group)
{
    const char **gports;

    if (client) {
	gports = jack_get_ports(client, group, JACK_DEFAULT_AUDIO_TYPE,
				JackPortIsInput);
	if (gports) {
	    int i, j;
	    for (i = 0; input_ports[i]; ++i) {
		for (j = 0; gports[j]; ++j) {
		    if (jack_port_connected_to(output_ports[i],
					       gports[j])) {
			free(gports);
			return TRUE;
		    }
		}
	    }
	    free(gports);
	}
    }
    return FALSE;
}


/* add a group of JACK ports to the interface
 *
 *  Creates a menu item for the `group' with `handler' as callback.
 *  The `check' function looks at either the input or output ports to
 *  see if any are already connected to that `group'.
 */
static GtkWidget *
iomenu_group_item(iomenu_check check, iomenu_callback handler,
		  const char *group)
{
    GtkWidget *item = gtk_check_menu_item_new_with_label(group);
    
    /* if any of these ports are already connected, check the item */
    if (check(group))
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);

    g_signal_connect(G_OBJECT(item), "activate",
		     G_CALLBACK(handler), (char *) group);

    return item;
}

/* add groups item and submenu followed by separator */
GtkWidget *
iomenu_add_groups(GtkWidget *menu, const char **groups,
		  iomenu_check check, iomenu_callback handler)
{
    GtkWidget *item =
	iomenu_add_item(menu, gtk_menu_item_new_with_label(_("Groups")));
    GtkWidget *sub_menu = iomenu_add_submenu(item);
    int i;

    /* create menu items for each input group */
    for (i = 0; groups[i]; ++i) {
	iomenu_add_item(sub_menu,
			iomenu_group_item(check, handler, groups[i]));
    }
    gtk_widget_show(sub_menu);

    iomenu_add_item(menu, gtk_separator_menu_item_new());

    return item;
}

/* add a local JACK port to the interface
 *
 *  Creates a menu item for the `port'.  Attaches a submenu to this
 *  menu item containing a list of all the JACK ports to which this
 *  local port could possibly be connected.
 */
static GtkWidget *
iomenu_port_item(jack_port_t *port)
{
    GtkWidget *item;			/* menu item for port */
    GtkWidget *sub_menu;		/* connection submenu for this port */
    const char *port_name;

    port_name = jack_port_short_name(port);
    item = gtk_menu_item_new_with_label(port_name);
    sub_menu = iomenu_add_submenu(item);
    if (jack_port_flags(port) & JackPortIsInput) {
	int i;
	for (i = 0; outports[i]; ++i) {
	    iomenu_add_item(sub_menu,
			    iomenu_connection_item(port, outports[i]));
	}
    } else if (jack_port_flags(port) & JackPortIsOutput) {
	int i;
	for (i = 0; inports[i]; ++i) {
	    iomenu_add_item(sub_menu,
			    iomenu_connection_item(port, inports[i]));
	}
    }
    gtk_widget_show(sub_menu);
    return item;
}

/* copy colon-terminated and NULL-terminate group_name */
static inline char *
iomenu_copy_name(const char *p)
{
    char *ret = &group_names[ngroup_names];

    /* there is always enough room in group_names[] */
    while (*p != ':')
	group_names[ngroup_names++] = *p++;
    
    group_names[ngroup_names++] = ':';
    group_names[ngroup_names++] = '\0';

    return ret;
}

/* compare colon-terminated names */
static inline int
iomenu_name_matches(const char *p, const char *q)
{
    do {
	if (*p == ':')
	    return (*q == ':');
    } while (*p++ == *q++);

    return FALSE;
}

/* look up colon-terminated name in group list */
static int
iomenu_lookup_name(const char **groups, int ngroups, const char *name)
{
    int i;

    /* the last entry is the most likely match, so search the list
     * backwards */
    for (i = ngroups-1; i >= 0; --i) {
	if (iomenu_name_matches(groups[i], name))
	    return TRUE;
    }

    return FALSE;
}

/* build NULL-terminated list of unique client names in ports list */
static void
iomenu_list_groups(const char **groups, const char **ports)
{
    int ngroups = 0;
    int i;

    for (i = 0; ports[i] && (ngroups < MAXGROUPS-1); ++i) {
	if (!iomenu_lookup_name(groups, ngroups, ports[i]))
	    groups[ngroups++] = iomenu_copy_name(ports[i]);
    }
    groups[ngroups] = NULL;
}

/* make an up-to-date list of JACK input and output port names
 *
 *   returns: 0 if successful, nonzero otherwise.
 */
static int
iomenu_list_jack_ports()
{
    if (client == NULL)			/* not connected to JACK? */
	return ESRCH;

    if (inports)
	free(inports);
    inports = jack_get_ports(client, NULL, JACK_DEFAULT_AUDIO_TYPE,
			     JackPortIsInput);
    if (inports == NULL)
	return ENOENT;

    if (outports)
	free(outports);
    outports = jack_get_ports(client, NULL, JACK_DEFAULT_AUDIO_TYPE,
			      JackPortIsOutput);
    if (outports == NULL)
	return ENOENT;

    ngroup_names = 0;			/* empty group name buffer */
    iomenu_list_groups(ingroups, inports);
    iomenu_list_groups(outgroups, outports);

    return 0;
}

/* called whenever the port menu pulldown is selected */
void
iomenu_pull_down_ports()
{
    int i;

    if (iomenu_list_jack_ports() != 0)	/* not connected to JACK? */
	return;

    /* recreate port menu widgets, connect to menubar items */
    if (iports_menu) {
	gtk_widget_destroy(iports_menu);
	gtk_widget_destroy(oports_menu);
    }
    iports_menu = iomenu_add_submenu(in_item);
    oports_menu = iomenu_add_submenu(out_item);

    /* add input groups item and submenu, plus separator */
    iomenu_add_groups(iports_menu, outgroups,
		      iomenu_check_input, iomenu_all_inputs);

    /* create menu items for each input port */
    for (i = 0; input_ports[i]; ++i) {
	iomenu_add_item(iports_menu,
			iomenu_port_item(input_ports[i]));
    }

    /* add output groups item and submenu, plus separator */
    iomenu_add_groups(oports_menu, ingroups,
		      iomenu_check_output, iomenu_all_outputs);

    /* create menu items for each output port */
    for (i = 0; output_ports[i]; ++i) {
	iomenu_add_item(oports_menu,
			iomenu_port_item(output_ports[i]));
    }

    gtk_widget_show(iports_menu);
    gtk_widget_show(oports_menu);
}

/* initialization */
void 
iomenu_bind(GtkWidget *main_window)
{
    /* Allocate buffer for up to MAXGROUPS input and output group
     * names.  Each group name is a JACK client name followed by ':'
     * and a zero byte. */
    group_names = malloc(2 * MAXGROUPS * (jack_client_name_size() + 1));

    /* find JACK Ports menubar item */
    menubar_item = lookup_widget(main_window, "jack_ports");

    /* Build Ports menu with Connect and Disconnect items.  The actual
     * submenus are not created until the pulldown is selected. */
    pulldown_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menubar_item), pulldown_menu);
    in_item =
	iomenu_add_item(pulldown_menu,
			gtk_menu_item_new_with_label(_("In")));
    out_item =
	iomenu_add_item(pulldown_menu,
			gtk_menu_item_new_with_label(_("Out")));
    gtk_widget_show(pulldown_menu);
}
