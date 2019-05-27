/*
 *  Copyright (C) 2003 Jack O'Quin
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
 *  $Id: resource.c,v 1.1 2004/01/10 05:19:18 joq Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gtk/gtk.h>

#include "main.h"
#include "resource.h"

static const char *resource_file;	/* GTK resource file, -r option */

/* Name associated with -r option, NULL if none */
void resource_file_name(const char *name)
{
    if (name)
	resource_file = name;
    else
	resource_file = JAMIN_EXAMPLES_DIR JAMIN_UI;
}

void resource_file_parse(void)
{
    if (resource_file) {

	/* use resource file specified with -r option */
	printf("Using GTK resource file %s\n", resource_file);
	gtk_rc_parse(resource_file);

    } else if (jamin_dir) {

	char rcfile[PATH_MAX];
	int fd;

	/* look for a user-defined GTK rc file, and parse it */
	snprintf(rcfile, PATH_MAX, "%s%s", jamin_dir, JAMIN_UI);
	if ((fd = open(rcfile, O_RDONLY)) >= 0) {
	    close(fd);
	    gtk_rc_parse(rcfile);
	}
    }
}
