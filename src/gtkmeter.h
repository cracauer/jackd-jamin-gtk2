/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2003 Steve Harris
 * Copyright (C) 2013 Patrick Shirkey
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __GTK_METER_H__
#define __GTK_METER_H__


#include <gdk/gdk.h>
//include <gtk/gtkadjustment.h>
//#include <gtk/gtkwidget.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


G_BEGIN_DECLS

#define GTK_TYPE_METER            (gtk_meter_get_type ())
#define GTK_METER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_METER, GtkMeter))
#define GTK_METER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_METER, GtkMeterClass))
#define GTK_IS_METER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_METER))
#define GTK_IS_METER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_METER))
#define GTK_METER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_METER, GtkMeterClass))


#define GTK_METER_UP    0
#define GTK_METER_DOWN  1
#define GTK_METER_LEFT  2
#define GTK_METER_RIGHT 3

#define GTK_METERSCALE_LEFT    1
#define GTK_METERSCALE_RIGHT   2
#define GTK_METERSCALE_TOP     4
#define GTK_METERSCALE_BOTTOM  8

typedef struct _GtkMeter        GtkMeter;
typedef struct _GtkMeterPrivate        GtkMeterPrivate;
typedef struct _GtkMeterClass   GtkMeterClass;

struct _GtkMeter
{
  GtkWidget widget;
  
  GtkMeterPrivate *priv;
  
};

struct _GtkMeterClass
{
  GtkWidgetClass parent_class;
};


GType          gtk_meter_get_type               (void) G_GNUC_CONST;
GtkWidget*     gtk_meter_new                    (GtkAdjustment *adjustment,
						 gint direction,
						 gint sides,
						 gfloat min,
						 gfloat max);

GtkAdjustment* gtk_meter_get_adjustment         (GtkMeter     *meter);

void           gtk_meter_set_adjustment         (GtkMeter     *meter,
						 GtkAdjustment *adjustment);

float          gtk_meter_get_peak               (GtkMeter *meter);

void	       gtk_meter_reset_peak		(GtkMeter     *meter);

void           gtk_meter_set_warn_point         (GtkMeter *meter,
						 gfloat pt);

#ifdef __cplusplus
}
#endif /* __cplusplus */

G_END_DECLS

#endif /* __GTK_METER */
