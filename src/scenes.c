/*
 *  scenes.c -- Scene changing buttons for the JAMin (JACK Audio Mastering 
 *              interface) program.
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


/*

    A few programmer notes:

    We're trying to stay away from extern'ed global variables as much as
    possible (I've been tainted by C++  ;-)  If you need to access a variable
    that is used here (set or get) write a liitle one line function that
    returns or sets it.  You can call it scenes_set_... or scenes_get_...  Yes,
    there is some overhead associated with it but it makes tracking things
    much easier.

    Scene numbers for scenes that have been modified are set to the scene
    number plus 100 (yeah, I know, it's kinda cheesy but it works).  A scene
    number of -1 means use whatever scene is current or last had its button
    pressed.

*/


#include <glib.h>

#include "scenes.h"
#include "state.h"
#include "main.h"
#include "support.h"
#include "interface.h"
#include "hdeq.h"


static GtkMenu           *scene_menu;
static GtkImage          *l_scene[NUM_SCENES], *buttons[4];
static GtkEventBox       *l_scene_eventbox[NUM_SCENES];
static char              l_scene_name[NUM_SCENES][100];
static int               current_scene = -1, menu_scene, prev_scene = -999;
static gboolean          scene_loaded[NUM_SCENES];
static s_state           scene_state[NUM_SCENES];
static GdkPixbuf         *LED_green_on = NULL, *LED_green_off = NULL, 
                         *LED_yellow = NULL, *LED_red = NULL;


/*  Initialize all scene related structures and get the widget addresses.  */

void bind_scenes ()
{
    int             i, j;
    char            *name;


 //   GtkTooltips *tooltips = gtk_tooltips_new();

    name = malloc(sizeof(char) * 32);

    scene_menu = (GtkMenu *) create_scene_menu();

    current_scene = -1;
    menu_scene = -1;


    /*  Looking up the widgets we'll need to work with based on the name
        that was set in glade-2.  If you change the widget name in glade-2
        you'll break the app.  */

    for (i = 0 ; i < NUM_SCENES ; i++)
      {
        sprintf (name, "scene%d", i + 1);
        l_scene[i] = GTK_IMAGE (lookup_widget (main_window, name));

        scene_state[i].description = name;

        sprintf (name, "scene%d_eventbox", i + 1);
        l_scene_eventbox[i] = 
          GTK_EVENT_BOX (lookup_widget (main_window, name));

        scene_loaded[i] = FALSE;


        /*  Initialize the scene states.  */

        for (j = 0 ; j < S_SIZE ; j++) scene_state[i].value[j] = 0.0;

        scene_state[i].value[S_NOTCH_Q(1)] = 5.0;
        scene_state[i].value[S_NOTCH_Q(2)] = 5.0;
        scene_state[i].value[S_NOTCH_Q(3)] = 5.0;

        scene_state[i].value[S_NOTCH_FREQ(0)] = 29.0;
        scene_state[i].value[S_NOTCH_FREQ(1)] = 131.0;
        scene_state[i].value[S_NOTCH_FREQ(2)] = 710.0;
        scene_state[i].value[S_NOTCH_FREQ(3)] = 3719.0;
        scene_state[i].value[S_NOTCH_FREQ(4)] = 16903.0;


        gtk_widget_set_tooltip_text ( GTK_WIDGET (l_scene_eventbox[i]), 
                              g_strdup_printf 
                              ("Scene %d, right click for menu", i + 1));
      }

    free(name);


    /*  We're cheating here.  I've set the first four images in the glade scene
        "buttons" to be green_on, green_off, yellow, and red so I can grab
        easily.  Don't change them in glade!  JCD  */

    for (i = 0 ; i < 4 ; i++)
      buttons[i] = GTK_IMAGE (gtk_image_new_from_pixbuf (gtk_image_get_pixbuf 
                                                         (l_scene[i])));

    LED_green_on = gtk_image_get_pixbuf (buttons[0]);
    LED_green_off = gtk_image_get_pixbuf (buttons[1]);
    LED_yellow = gtk_image_get_pixbuf (buttons[2]);
    LED_red = gtk_image_get_pixbuf (buttons[3]);
}


/*  Select one of the scenes as the current scene or pop up the set/clear
    menu.  */

void select_scene (int number, int button)
{
    int             i, j;
    gboolean        warning;


    /*  Check for modified scene.  */

    warning = FALSE;
    if (number > 99)
      {
        number -= 100;
        warning = TRUE;
      }


    switch (button)
      {
        /*  Left button selects the scene.  */

      case 1:
        if (scene_loaded[number])
          {
            for (i = 0 ; i < NUM_SCENES ; i++)
              {
                /*  The matching scene.  */

                if (i == number)
                  {
                    if (warning)
                      {
                        gtk_image_set_from_pixbuf (l_scene[i], LED_yellow);

                        current_scene = number;
                      }
                    else
                      {
                        current_scene = i;


                        /*  This is a fix for an earlier screwup that may or
                            may not exist in some saved .jam files.  Basically,
                            the default notch frequencies were set to 0.0
                            which is way wrong and causes problems during
                            crossfading.  This code just checks them and 
                            resets them to the default if they aren't set
                            correctly.  */

                        /****************************************************/

                        for (j = 0 ; j < NOTCHES ; j++)
                          {
                            if (scene_state[i].value[(S_NOTCH_FREQ(j))] == 0.0)
                              {
                                scene_state[i].value[S_NOTCH_FREQ(j)] = 
         //                         hdeq_get_notch_default_freq (j);

                                scene_state[i].value[S_NOTCH_Q(j)] = 5.0;
                                scene_state[i].value[S_NOTCH_GAIN(j)] = 0.0;

                                if (!j || j == NOTCHES - 1)
                                    scene_state[i].value[S_NOTCH_Q(j)] = 0.0;
                              }
                          }

                        /****************************************************/


                        s_crossfade_to_state (&scene_state[i], -1.0f);

        //                set_EQ_curve_values (0, 0.0);

                        s_history_add_state (scene_state[i]);

                        gtk_image_set_from_pixbuf (l_scene[i], LED_green_on);
                      }
                  }


                /*  Non-matching scenes - set to green off or red if not 
                    loaded.  */

                else
                  {
                    if (scene_loaded[i])
                      {
                        gtk_image_set_from_pixbuf (l_scene[i], LED_green_off);
                      }
                    else
                      {
                        gtk_image_set_from_pixbuf (l_scene[i], LED_red);
                      }
                  }
              }
          }
        break;


        /*  Middle button is ignored.  */

      case 2:
        break;


        /*  Right button pops up the set/clear menu.  */

      case 3:
        menu_scene = number % 100;
        gtk_menu_popup (scene_menu, NULL, NULL, NULL, NULL, button, 
                        gtk_get_current_event_time());
        break;
      }
}


/*  Returns the current active scene number or -1 if no scene is active.  */

int get_current_scene ()
{
    return (current_scene);
}


/*  Returns the requested scene state or NULL if that scene is not loaded.  */

s_state *get_scene (int number)
{
  int       i;


  i = number % 100;

  if (!scene_loaded[i]) return (NULL);

  return (&scene_state[i]);
}

void scene_init()
{
    memset(scene_state, 0, sizeof(s_state) * NUM_SCENES);
}


/*  Set the scene state from the current settings.  Get the scene name from
    the scene_name text entry widget.  If scene_num is -1 use the last pressed
    scene button number (menu_scene).  */

void set_scene (int scene_num)
{
    int         i;

 //   GtkTooltips *tooltips = gtk_tooltips_new();



    /*  Only save the scene settings if we're going from the current settings.
        That is, scene_num = -1.  Otherwise we may be in the middle of 
        crossfading to a new state.  */

    if (scene_num >= 0) menu_scene = prev_scene = scene_num;


    /*  Grab the current state.  */

    for (i = 0 ; i < S_SIZE ; i++) 
      scene_state[menu_scene].value[i] = s_get_value(i);

    scene_state[menu_scene].description = 
      (char *) realloc (scene_state[menu_scene].description, 
                        strlen (l_scene_name[menu_scene]) + 1);

    strcpy (scene_state[menu_scene].description, l_scene_name[menu_scene]);


    /*  Set the scene loaded flag.  */

    scene_loaded[menu_scene] = TRUE;


    /*  Change the selected icon to green.  */

    for (i = 0 ; i < NUM_SCENES ; i++)
      {
        /*  Matching scene.  */

        if (i == menu_scene)
          {
            gtk_image_set_from_pixbuf (l_scene[i], LED_green_on);

            current_scene = i;
          }


        /*  Non-matching scene - set to green off or red.  */

        else
          {
            if (scene_loaded[i])
              {
                gtk_image_set_from_pixbuf (l_scene[i], LED_green_off);
              }
            else
              {
                gtk_image_set_from_pixbuf (l_scene[i], LED_red);
              }
          }
      }


    /*  Set the tooltip to the full name.  */

    gtk_widget_set_tooltip_text ( GTK_WIDGET (l_scene_eventbox[menu_scene]), 
                          scene_state[menu_scene].description );
}


/* Gets the scene name for scene "number".  */

const char *get_scene_name (int number)
{
  int        i;


  i = number % 100;

  if (!scene_loaded[i]) return (NULL);
  return (l_scene_name[i]);
}


/*  Set the scene name.  If the scene_name passed in is null get the name 
    from the scene_name text entry widget.  This is called from callbacks.c
    on a change to the scene_name widget.  */

void set_scene_name (int number, const char *scene_name)
{
  //  GtkTooltips *tooltips = gtk_tooltips_new();
    int         i;


    i = number % 100;


    /*  If we are trying to modify the name without anything loaded, bypass
        this.  */

    if (menu_scene < 0) return;


    if (scene_name != NULL)
        strcpy (l_scene_name[i], scene_name);

    scene_state[i].description = 
        (char *) realloc (scene_state[i].description, 
        strlen (l_scene_name[i]) + 1);

    strcpy (scene_state[menu_scene].description, l_scene_name[i]);


    /*  Set the tooltip to the name.  */

    gtk_widget_set_tooltip_text ( GTK_WIDGET (l_scene_eventbox[menu_scene]), 
                          scene_state[menu_scene].description );
}


/*  Clear the scene state.  If scene_num is -1 use the last pressed scene 
    button number.  */

void clear_scene (int scene_num)
{
    int         i;


 //   GtkTooltips *tooltips = gtk_tooltips_new();


    /*  Strip off the warning if set.  */

    i = scene_num % 100;


    if (i >= 0) menu_scene = i;


    gtk_widget_set_tooltip_text ( GTK_WIDGET (l_scene_eventbox[menu_scene]), 
                          g_strdup_printf ("Scene %d, right click for menu", 
                                           menu_scene + 1) );


    /*  Set the button to red.  */

    gtk_image_set_from_pixbuf (l_scene[menu_scene], LED_red);

    scene_loaded[menu_scene] = FALSE;


    /*  Resety the scene name to the default.  */

    strcpy (l_scene_name[menu_scene], g_strdup_printf ("Scene %d", 
                                                       menu_scene + 1));
}


/*  Set all of the buttons to unselected state.  This should be done whenever 
    there is a state change.  */

void unset_scene_buttons ()
{
    int         i;


    current_scene = -1;
    for (i = 0 ; i < NUM_SCENES ; i++)
      {
        gtk_image_set_from_pixbuf (l_scene[i], LED_red);

        scene_loaded[i] = FALSE;

        strcpy (l_scene_name[i], g_strdup_printf("Scene %d", i + 1));
      }
}


/*  This is used in state.c to find out if we need to undo/redo.  */

int get_previous_scene_num ()
{
  return (prev_scene);
}


/*  Set the current scene button to a warning.  This is done whenever any 
    state changes are made while a scene is active.  */

void set_scene_warning_button ()
{
  int        i;


  i = current_scene % 100;

  if (current_scene < 100 && current_scene > -1)
    {
      prev_scene = i;

      gtk_image_set_from_pixbuf (l_scene[i], LED_yellow);
      current_scene = changed_scene_no(i);
    }
}


/*  Set the specified scene button to active.  Only done on undo/redo.  */

void set_scene_button (int scene)
{
    int i;


    /*  Change the selected icon to green/yes.  */

    for (i = 0 ; i < NUM_SCENES ; i++)
      {
        if (i == scene)
          {
            gtk_image_set_from_pixbuf (l_scene[i], LED_green_on);

            current_scene = i;
          }
        else
          {
            if (scene_loaded[i])
              {
                gtk_image_set_from_pixbuf (l_scene[i], LED_green_off);
              }
            else
              {
                gtk_image_set_from_pixbuf (l_scene[i], LED_red);
              }
          }
      }
}


/* Return the magic scene number that will be used to represent that scene if 
   it has had unsaved changes made */

int changed_scene_no(int s)
{
	return s + 100;
}


/* return TRUE if the scene number passed in represents a changed scene and
   FALSE otherwise */

int is_changed_scene(int s)
{
	return s >= 100;
}

/*  Set a specific scene button to a warning.  Only done on load.  */

void set_num_scene_warning_button (int scene)
{
  int        i;


  i = scene % 100;


  gtk_image_set_from_pixbuf (l_scene[i], LED_yellow);
}
