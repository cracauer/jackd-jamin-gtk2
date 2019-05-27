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
 *  $Id: gtkmeterscale.h,v 1.2 2003/11/19 15:28:17 theno23 Exp $
 */

#ifndef __GTK_METERSCALE_H__
#define __GTK_METERSCALE_H__


#include <gdk/gdk.h>
#include <gtk/gtk.h>
//#include <gtk/gtkwidget.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_METERSCALE(obj)          GTK_CHECK_CAST (obj, gtk_meterscale_get_type (), GtkMeterScale)
#define GTK_METERSCALE_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gtk_meterscale_get_type (), GtkMeterScaleClass)
#define GTK_IS_METERSCALE(obj)       GTK_CHECK_TYPE (obj, gtk_meterscale_get_type ())

#define GTK_METERSCALE_LEFT    1
#define GTK_METERSCALE_RIGHT   2
#define GTK_METERSCALE_TOP     4
#define GTK_METERSCALE_BOTTOM  8

typedef struct _GtkMeterScale        GtkMeterScale;
typedef struct _GtkMeterScaleClass   GtkMeterScaleClass;

struct _GtkMeterScale
{
  GtkWidget widget;

  /* the sides scales are marked on */
  guint direction;

  /* Deflection limits */
  gfloat lower;
  gfloat upper;
  gfloat iec_lower;
  gfloat iec_upper;

  int min_width;
  int min_height;
};

struct _GtkMeterScaleClass
{
  GtkWidgetClass parent_class;
};


GtkWidget*     gtk_meterscale_new               (gint direction,
						 gfloat min,
						 gfloat max);

guint          gtk_meterscale_get_type          (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_METERSCALE */
