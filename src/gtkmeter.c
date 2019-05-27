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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <math.h>
#include <stdio.h>
#include <gtk/gtk.h>


#include "gtkmeter.h"

#define METER_DEFAULT_WIDTH 10
#define METER_DEFAULT_LENGTH 100
#define METERSCALE_MAX_FONT_SIZE 8

/* Forward declarations */

static void gtk_meter_class_init               (GtkMeterClass    *klass);
static void gtk_meter_init                     (GtkMeter         *meter);
static void gtk_meter_set_property   (GObject          *object,
                                      guint             prop_id,
                                      const GValue     *value,
                                      GParamSpec       *pspec);
static void gtk_meter_get_property   (GObject          *object,
                                      guint             prop_id,
                                      GValue           *value,
                                      GParamSpec       *pspec);


static void gtk_meter_destroy                  (GtkWidget        *widget);
static void gtk_meter_realize                  (GtkWidget        *widget);
static void gtk_meter_size_allocate            (GtkWidget     *widget,
				 	        GtkAllocation *allocation);
static gboolean gtk_meter_draw       (GtkWidget        *widget,
                                      cairo_t          *cr);
static void create_meter(GtkWidget *widget, cairo_t *cr);           
static void rotate_widget(GtkMeterPrivate *priv, cairo_t *cr, int length, int width);                           
static void meterscale_draw_notch_label(GtkMeter *meterscale, float db,
		int mark, PangoRectangle *last_label_rect);
static void draw_notch(GtkMeter *meter, GtkMeterPrivate * priv, cairo_t * cr, float db,
		int mark, int length, int width);						
static void gtk_meter_update                   (GtkMeter *meter);
static void gtk_meter_adjustment_changed       (GtkAdjustment    *adjustment,
						gpointer          data);
static void gtk_meter_adjustment_value_changed (GtkAdjustment    *adjustment,
						gpointer          data);

static float iec_scale(float db);


G_DEFINE_TYPE_WITH_CODE (GtkMeter, gtk_meter, GTK_TYPE_WIDGET,
                                  G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE,
                                                         NULL))                                              
                                               
/* Local data */

static GtkWidgetClass *parent_class = NULL;

  
struct _GtkMeterPrivate
{  
  GtkOrientation orientation;
  
  /* The adjustment object that stores the data for this meter */
  GtkAdjustment *adjustment;
    
  /* Deflection limits */
  gfloat lower;
  gfloat upper;
  gfloat iec_lower;
  gfloat iec_upper;
  
  int min_width;
  int min_height;

  /* update policy (GTK_UPDATE_[CONTINUOUS/DELAYED/DISCONTINUOUS]) */
  guint direction : 2;
  
  GdkWindow         *event_window;
  
  /* the sides scales are marked on */
  guint sides;

  /* Button currently pressed or 0 if none */
  guint8 button;

  /* Amber dB and deflection points */
  gfloat amber_level;
  gfloat amber_frac;

  /* Peak deflection */
  gfloat peak;
  
  /* Peak deflection in reasonable units */
  gfloat peak_db;
  
  /* ID of update timer, or 0 if none */
  guint32 timer;

  /* Old values from adjustment stored so we know when something changes */
  gfloat old_value;
  gfloat old_lower;
  gfloat old_upper;


};

enum {
  PROP_0,
  PROP_ORIENTATION,
  PROP_ADJUSTMENT
};  

static void
gtk_meter_class_init (GtkMeterClass *class)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *widget_class;

//  printf("in gtk_meter_class_init\n");
  
  gobject_class = G_OBJECT_CLASS (class);
  widget_class = (GtkWidgetClass*) class;
	
  gobject_class->set_property = gtk_meter_set_property;
  gobject_class->get_property = gtk_meter_get_property;
  
 // widget_class->destroy = gtk_meter_destroy;
  widget_class->realize = gtk_meter_realize;
  widget_class->draw = gtk_meter_draw;
//  widget_class->expose_event = gtk_meter_expose;
//  widget_class->size_request = gtk_meter_size_request;
//  widget_class->size_allocate = gtk_meter_size_allocate;

  g_object_class_override_property (gobject_class,
                                    PROP_ORIENTATION,
                                    "orientation");
                                    
 g_object_class_install_property (gobject_class,
                                   PROP_ADJUSTMENT,
                                   g_param_spec_object ("adjustment",
							"Adjustment",
							"The GtkAdjustment that contains the current value of this meter object",
                                                        GTK_TYPE_ADJUSTMENT,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
                                                                      
  g_type_class_add_private (gobject_class, sizeof (GtkMeterPrivate));                                 
  
//  printf("leaving gtk_meter_class_init\n");                                  

}

static void
gtk_meter_init (GtkMeter *meter)
{

//  printf("in gtk_meter_init\n");

  GtkMeterPrivate *priv;

  meter->priv = G_TYPE_INSTANCE_GET_PRIVATE (meter,
                                             GTK_TYPE_METER,
                                             GtkMeterPrivate);
  priv = meter->priv;

  gtk_widget_set_has_window (GTK_WIDGET (meter), FALSE);	
	
 // meter = G_TYPE_INSTANCE_GET_CLASS (meter,GTK_TYPE_METER,GtkMeter);	

  priv->orientation = GTK_ORIENTATION_VERTICAL;
  priv->adjustment = NULL;	
  priv->button = 0;
  priv->direction = GTK_METER_UP;
  priv->sides = GTK_METERSCALE_LEFT;
  priv->timer = 0;
  priv->amber_level = -6.0f;
  priv->amber_frac = 0.0f;
  priv->iec_lower = 0.0f;
  priv->iec_upper = 0.0f;
  priv->min_width = -1;
  priv->min_height = -1;
  priv->old_value = 0.0;
  priv->old_lower = 0.0;
  priv->old_upper = 0.0;

  
  
  
}

static void
gtk_meter_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  GtkMeter *meter = GTK_METER (object);
  GtkMeterPrivate *priv = meter->priv;
  
//  printf("in gtk_meter_set_property\n");
  
  switch (prop_id)
    {
    case PROP_ORIENTATION:
      priv->orientation = g_value_get_enum (value);

     // _gtk_orientable_set_style_classes (GTK_ORIENTABLE (meter));
      gtk_widget_queue_resize (GTK_WIDGET (meter));
      break;
    case PROP_ADJUSTMENT:
      gtk_meter_set_adjustment (meter, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_meter_get_property (GObject      *object,
			guint         prop_id,
			GValue       *value,
			GParamSpec   *pspec)
{
  GtkMeter *meter = GTK_METER (object);
  GtkMeterPrivate *priv = meter->priv;

// printf("in gtk_meter_get_property\n");

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    case PROP_ADJUSTMENT:
      g_value_set_object (value, priv->adjustment);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


GtkWidget*
gtk_meter_new (GtkAdjustment *adjustment, gint direction, gint sides, gfloat min, gfloat max)
{
	GtkMeter *meter;
	GtkMeterPrivate *priv;
//  printf("in gtk_meter_new\n");

 // if (!adjustment)
    adjustment = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

	GObject *newObject = g_object_new (GTK_TYPE_METER, NULL);
	meter = GTK_METER (newObject);
	priv = meter->priv;
	
  	priv->adjustment = adjustment;
	priv->direction = direction;
	priv->sides = sides;
	priv->lower = min;
	priv->upper = max;
	priv->iec_lower = iec_scale(min);
	priv->iec_upper = iec_scale(max);
				
	return newObject;
}

GtkAdjustment*
gtk_meter_get_adjustment (GtkMeter *meter)
{
  GtkMeterPrivate *priv;

  g_return_val_if_fail (GTK_IS_METER (meter), NULL);

  priv = meter->priv;

  if (!priv->adjustment)
    gtk_meter_set_adjustment (meter, NULL);

  return priv->adjustment;
}

void
gtk_meter_set_adjustment (GtkMeter      *meter,
			  GtkAdjustment *adjustment)
{
  GtkMeterPrivate *priv;

  g_return_if_fail (GTK_IS_METER (meter));

  priv = meter->priv;

  if (!adjustment)
    adjustment = gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  else
    g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  if (priv->adjustment != adjustment)
    {
      if (priv->adjustment)
	{
	  g_signal_handlers_disconnect_by_func (priv->adjustment,
						gtk_meter_adjustment_changed,
						meter);
	  g_signal_handlers_disconnect_by_func (priv->adjustment,
						gtk_meter_adjustment_value_changed,
						meter);
	  g_object_unref (priv->adjustment);
	}

      priv->adjustment = adjustment;
      g_object_ref_sink (adjustment);

      g_signal_connect (adjustment, "changed",
			G_CALLBACK (gtk_meter_adjustment_changed),
			meter);
      g_signal_connect (adjustment, "value-changed",
			G_CALLBACK (gtk_meter_adjustment_value_changed),
			meter);

      gtk_meter_adjustment_changed (adjustment, meter);
      g_object_notify (G_OBJECT (meter), "adjustment");
    }
    
    gtk_meter_update (meter);
    
}  

static void
gtk_meter_realize (GtkWidget *widget)
{
	
 GtkAllocation allocation;
  GtkMeter *meter= GTK_METER (widget);
  GtkMeterPrivate *priv = meter->priv;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;

 // gtk_range_calc_layout (meter, gtk_adjustment_get_value (priv->adjustment));

  gtk_widget_set_realized (widget, TRUE);

  window = gtk_widget_get_parent_window (widget);
  gtk_widget_set_window (widget, window);
  g_object_ref (window);

  gtk_widget_get_allocation (widget, &allocation);
 // if (modify_allocation_for_window_grip (widget, &allocation))
   // gtk_widget_set_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
                            GDK_BUTTON_RELEASE_MASK |
                            GDK_SCROLL_MASK |
                            GDK_SMOOTH_SCROLL_MASK |
                            GDK_ENTER_NOTIFY_MASK |
                            GDK_LEAVE_NOTIFY_MASK |
                            GDK_POINTER_MOTION_MASK |
                            GDK_POINTER_MOTION_HINT_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y;

  priv->event_window = gdk_window_new (gtk_widget_get_parent_window (widget),
					&attributes, attributes_mask);
  gdk_window_set_user_data (priv->event_window, meter);	
	


}	
	
/*	

static void 
gtk_meter_size_request (GtkWidget     *widget,
		       GtkRequisition *requisition)
{
  GtkMeter *meter = GTK_METER(widget);

  if (meter->direction == GTK_METER_UP || meter->direction == GTK_METER_DOWN) {
    requisition->width = METER_DEFAULT_WIDTH;
    requisition->height = METER_DEFAULT_LENGTH;
  } else {
    requisition->width = METER_DEFAULT_LENGTH;
    requisition->height = METER_DEFAULT_WIDTH;
  }
}

static void
gtk_meter_size_allocate (GtkWidget    *widget,
			GtkAllocation *allocation)
{
  GtkMeter *meter;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_METER (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;
  meter = GTK_METER (widget);

  if (GTK_WIDGET_REALIZED (widget))
    {

      gdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

    }
}
*/

static gboolean
gtk_meter_draw (GtkWidget *widget,
                cairo_t   *cr)
{
 
//  GtkMeter *meter;
  GtkMeter *meter = GTK_METER (widget);
  GtkMeterPrivate *priv = meter->priv;
  
//  printf("in gtk_meter_draw\n");

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_METER (widget), FALSE);

  create_meter(widget, cr);

  
  return FALSE;
}

static void create_meter(GtkWidget *widget, cairo_t *cr)
{
	
  float val, frac, peak_frac;
  int g_h, a_h, r_h;
  int length = 0, width = 0;
  gboolean vert = TRUE;
 // cairo_t *cr;
  cairo_pattern_t *pat;	
  PangoRectangle lr;
  GtkAllocation *allocation;	  

  GtkMeter *meter = GTK_METER (widget);
  GtkMeterPrivate *priv = meter->priv;


  allocation = g_new0 (GtkAllocation, 1);

  gtk_widget_get_allocation(GTK_WIDGET(meter), allocation);



  switch (priv->direction) {
    case GTK_METER_UP:
    case GTK_METER_DOWN:
      length = allocation->height - 2;
      width = allocation->width - 2;
      break;
    case GTK_METER_LEFT:
    case GTK_METER_RIGHT:
      length = allocation->width - 2;
      width = allocation->height - 2;
      vert = FALSE;
      break;
  }

  val = iec_scale(gtk_adjustment_get_value(priv->adjustment));
  if (val > priv->peak) {
    if (val > priv->iec_upper) {
      priv->peak = priv->iec_upper;
    } else {
      priv->peak = val;
    }
  }

  frac = (val - priv->iec_lower) / (priv->iec_upper - priv->iec_lower);
  peak_frac = (priv->peak - priv->iec_lower) / (priv->iec_upper -
		  priv->iec_lower);

  /* Draw the background layer with gradient  */
  
    pat = cairo_pattern_create_linear (0.0, 0.0,  0.0, 256.0);
    cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0, 0, 1);
    cairo_pattern_add_color_stop_rgba (pat, 0, 0.6, 0.6, 0.6, 1);	
    if(vert)
		cairo_rectangle (cr, 0, 0, width, length);
	else
		cairo_rectangle (cr, 0, 0, length, width);
    cairo_set_source (cr, pat);
    cairo_fill (cr);
    cairo_pattern_destroy (pat);
//	rotate_widget(priv, cr, length, width);
		  

  if (frac < priv->amber_frac) {
    g_h = frac * length;
    a_h = g_h;
    r_h = g_h;
  } else if (val <= 100.0f) {
    g_h = priv->amber_frac * length;
    a_h = frac * length;
    r_h = a_h;
  } else {
    g_h = priv->amber_frac * length;
    a_h = length * (100.0f - priv->iec_lower) / (priv->iec_upper -
		    priv->iec_lower);
    r_h = frac * length;
  }

  if (a_h > length) {
    a_h = length;
  }
  if (r_h > length) {
    r_h = length;
  }


/* generate colors */

      // Green levels
      switch (priv->direction) {
		case GTK_METER_UP:
		case GTK_METER_DOWN:
		  cairo_rectangle (cr, 1, length - g_h + 1, width-1, g_h);
		  cairo_set_source_rgba (cr, 0.1, 0.5, 0.2, 0.8);
		  break;
		case GTK_METER_LEFT:
		  cairo_rectangle (cr,   a_h + 1, 1, length - a_h, width -1);
		  cairo_set_source_rgba (cr, 0.8,0.8,0.2, 0.8);
		  break;
		case GTK_METER_RIGHT:
		  cairo_rectangle (cr, 1, 1, g_h, width-1);	
		  cairo_set_source_rgba (cr, 0.1, 0.5, 0.2, 0.8);	  
		  break;          
		}

       // a set the opacity level
      cairo_fill (cr);
//		rotate_widget(priv, cr, length, width);
      
      
      if (a_h > g_h) {
	// amber levels
      switch (priv->direction) {
		case GTK_METER_UP:
		case GTK_METER_DOWN:
			cairo_rectangle (cr, 1, length - a_h + 1, width-1, a_h - g_h);
		  break;
		case GTK_METER_LEFT:
		//	cairo_rectangle (cr, length - a_h + 1, 1, length -(length - g_h), width-1);
		  break;
		case GTK_METER_RIGHT:
			cairo_rectangle (cr, g_h, 1, a_h -g_h ,  width-1);		
		  break;          
		}	

        cairo_set_source_rgba (cr, 0.8,0.8,0.2, 0.8); // a set the opacity level
        cairo_fill (cr);
//		rotate_widget(priv, cr, length, width);
			
      }
      if (r_h > a_h) {
	// red levels
      switch (priv->direction) {
		case GTK_METER_UP:
		case GTK_METER_DOWN:
			cairo_rectangle (cr, 1, length - r_h + 1, width-1, r_h - a_h);
		  break;
		case GTK_METER_LEFT:
		//  cairo_rectangle (cr, length - r_h + 1 , 1, length - (length - a_h), width-1);			
		  break;
		case GTK_METER_RIGHT:
		  cairo_rectangle (cr, a_h, 1,  r_h - a_h , width-1);	
		  break;          
		}		

        cairo_set_source_rgba (cr, 1,0,0.1, 0.8); // a set the opacity level
        cairo_fill (cr);
//		rotate_widget(priv, cr, length, width);
      }
      if (peak_frac > 0) {
        // peak levels
       switch (priv->direction) {
		case GTK_METER_UP:
		case GTK_METER_DOWN:
			cairo_rectangle (cr, 1, length * (1.0f - peak_frac) + 1, width, 1);
		  break;
		case GTK_METER_LEFT:
		//  cairo_rectangle (cr, length * (1.0f - peak_frac) + 1, 1, 1, width-1);			
		  break;
		case GTK_METER_RIGHT:
			cairo_rectangle (cr, length * peak_frac, 1, 1, width-1);
		  break;          
		}	       

        cairo_set_source_rgba (cr, 0.9,0.1,0.1, 0.7); // a set the opacity level
        cairo_fill (cr);
//		rotate_widget(priv, cr, length, width);
      }  
  
  
 // Create glassy layer effect

// left border

  if(vert)
	cairo_rectangle (cr, 0, 0, 2, length+2);
  else 
	cairo_rectangle (cr, 0, 0, 2, width+2);
  cairo_set_source_rgba (cr, 0.6,0.6,0.6, 1.0); 
  cairo_fill (cr);
//  rotate_widget(priv, cr, length, width);

  // top border
  if(vert)	 
	cairo_rectangle (cr, 2, 0, width+2, 1);
  else
	cairo_rectangle (cr, 2, 0, length+2, 1);	
  cairo_set_source_rgba (cr, 0.8,0.8,0.8, 1.0); 
  cairo_fill (cr);
 // rotate_widget(priv, cr, length, width);
 
// right border  
  if(vert)	 
	cairo_rectangle (cr, width-1, 1, 2, length+1);
  else 
	cairo_rectangle (cr, length-1, 1, 2, width+1);
  cairo_set_source_rgba (cr, 0.5,0.5,0.5, 1.0); 
  cairo_fill (cr);
//  rotate_widget(priv, cr, length, width);

// bottom border
  if(vert)
	cairo_rectangle (cr, 0, length, width-1, 2);
  else 
  	cairo_rectangle (cr, 0, width, length-1, 2);
  cairo_set_source_rgba (cr, 0.5,0.5,0.5, 1.0); 
  cairo_fill (cr);
//  rotate_widget(priv, cr, length, width);

 // left hand glass bubble effect  
  if(vert)
	pat = cairo_pattern_create_linear (width/2, 0, 0, 0);
  else
  	pat = cairo_pattern_create_linear (0, width/2, 0, 0);
    cairo_pattern_add_color_stop_rgba (pat, 0, 1.0, 1.0, 1.0, 0.3);
    cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0.2, 0.7, 0.2);
    if(vert)
		cairo_rectangle (cr, 0, 0, width/2, length);
	else
		cairo_rectangle (cr, 0, 0, length, width/2);	
    cairo_set_source (cr, pat);
    cairo_fill (cr);
    cairo_pattern_destroy (pat);
// rotate_widget(priv, cr, length, width);

 // right hand glass bubble effect
  if(vert)
	pat = cairo_pattern_create_linear (width/2, 0, width, 0);
  else
  	pat = cairo_pattern_create_linear ( 0, width/2, 0, width);
  cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0.4, 1, 0.2);	
  cairo_pattern_add_color_stop_rgba (pat, 0, 1.0, 1.0, 1.0, 0.3);
  if(vert)
	cairo_rectangle (cr, width/2, 0, width/2, length);
  else
	cairo_rectangle (cr,  0, width/2, length, width);
		
    cairo_set_source (cr, pat);
    cairo_fill (cr);
    cairo_pattern_destroy (pat);
//  rotate_widget(priv, cr, length, width);

  
  
 // lr.x = 0;
 // lr.y = 0;
 // lr.width = 0;
 // lr.height = 0;
  

  // meterscale_draw_notch_label(meter, 0.0f, 3, &lr);
  draw_notch(meter,priv, cr, 0.0f, 3, length, width);


  for (val = 5.0f; val < priv->upper; val += 5.0f) {
   // meterscale_draw_notch_label(meter, val, 2, &lr);
    draw_notch(meter,priv, cr, val, 2, length, width);
  }

  for (val = -5.0f; val > priv->lower+5.0f; val -= 5.0f) {
 //  meterscale_draw_notch_label(meter, val, 2, &lr);
     draw_notch(meter,priv, cr, val, 2, length, width);
  }
  
  for (val = -10.0f; val < priv->upper; val += 1.0f) {
    draw_notch(meter, priv, cr, val, 1, length, width);
  }
  
 //   g_object_unref (&lr);
  
 	
 	
}


static void rotate_widget(GtkMeterPrivate *priv, cairo_t *cr, int length, int width)
{
	
		
  switch (priv->direction) {
    case GTK_METER_LEFT:	
	//	printf("in gtk_meter_draw: left\n");
		cairo_translate (cr,  width * 0.5, length * 0.5);
		cairo_rotate (cr, 90* M_PI/180);
		cairo_translate (cr,  width * -0.5, length * -0.5);
		cairo_paint(cr);
      break;

    case GTK_METER_RIGHT:
	//	printf("in gtk_meter_draw: right\n");
		cairo_translate (cr, width * 0.5, length * 0.5);
		cairo_rotate (cr, 270* M_PI/180);
		cairo_translate (cr,  width * -0.5, length * -0.5);
		cairo_paint(cr);
      break;

  }
	
	
}

/*

static void meterscale_draw_notch_label(GtkMeter *meterscale, float db,
		int mark, PangoRectangle *last_label_rect)
{
    
    
    GtkWidget *widget = GTK_WIDGET(meterscale);
    int length, width, pos;
    int vertical = 0;

    if (meterscale->sides & GTK_METERSCALE_LEFT ||
        meterscale->sides & GTK_METERSCALE_RIGHT) {
	    length = widget->allocation.height - 2;
	    width = widget->allocation.width - 2;
	    pos = length - length * (iec_scale(db) - meterscale->iec_lower) /
		    (meterscale->iec_upper - meterscale->iec_lower);
	    vertical = 1;
    } else {
	    length = widget->allocation.width - 2;
	    width = widget->allocation.height - 2;
	    pos = length * (iec_scale(db) - meterscale->iec_lower) /
		    (meterscale->iec_upper - meterscale->iec_lower);
    }


    if (last_label_rect) {
 //       PangoContext *pc = gtk_widget_get_pango_context(widget);
        PangoLayout *pl;
//	PangoFontDescription *pfd;
	PangoRectangle rect;
	char text[3];
	int x, y, size;

	size = (6 + length / 150);
	if (size > METERSCALE_MAX_FONT_SIZE) {
	    size = METERSCALE_MAX_FONT_SIZE;
	}

//	pl = pango_layout_new(pc);
//	g_object_unref (pc);
	
/*	snprintf(text, 3, "%.0f", fabs(db));
	pl = gtk_widget_create_pango_layout(widget,text);
	
//	pango_layout_set_text(pl, text, -1);
	pango_layout_get_pixel_extents(pl, NULL, &rect);
	
        pfd = pango_font_description_new();
	pango_font_description_set_family(pfd, "sans");
	pango_font_description_set_size(pfd, size * PANGO_SCALE);

        pango_layout_set_font_description(pl, pfd);
*/        
/*        
        
      GtkStyle *style = gtk_widget_get_style(widget);
     
      snprintf(text, 3, "%.0f", fabs(db));
 //     pango_font_description_set_family(style->font_desc, "sans");
//      pango_font_description_set_size(style->font_desc, METERSCALE_MAX_FONT_SIZE * PANGO_SCALE);

//      gtk_widget_modify_font(widget, style->font_desc);
     // pl = pango_layout_new(gtk_widget_get_pango_context(widget));
      pl = gtk_widget_create_pango_layout(widget,text);
      
   //   gtk_widget_set_text(widget, text);
   //   pango_layout_set_text(pl, text, -1);
      pango_layout_get_pixel_extents(pl, &rect, NULL);        


	if (vertical) {
	    x = width/2 - rect.width/2 + 1;
	    y = pos - rect.height/2;
	    if (y < 1) {
		y = 1;
	    }
	} else {
	    x = pos - rect.width/2 + 1;
	    y = width/2 - rect.height / 2 + 1;
	    if (x < 1) {
		x = 1;
	    } else if (x + rect.width > length) {
	        x = length - rect.width + 1;
	    }
	}

	if (vertical && last_label_rect->y < y + rect.height + 2 && last_label_rect->y + last_label_rect->height + 2 > y) {
	    return;
	}
	if (!vertical && last_label_rect->x < x + rect.width + 2 && last_label_rect->x + last_label_rect->width + 2 > y) {
	    return;
	}

	gdk_draw_layout(widget->window, widget->style->black_gc, x, y, pl);
	last_label_rect->width = rect.width;
	last_label_rect->height = rect.height;
	last_label_rect->x = x;
	last_label_rect->y = y;
	
//	pango_font_description_free(pfd);	
	g_object_unref (pl);

//        g_object_unref (pc);	
    }

    meterscale_draw_notch(meterscale, db, mark);
    
    
}
*/
static void draw_notch(GtkMeter *meter, GtkMeterPrivate * priv, cairo_t * cr, float db,
		int mark, int length, int width)
{
    int pos;

    if (priv->sides & GTK_METERSCALE_LEFT ||
        priv->sides & GTK_METERSCALE_RIGHT) {
	    pos = length - length * (iec_scale(db) - priv->iec_lower) /
		    (priv->iec_upper - priv->iec_lower);
    } else {
	    pos = length * (iec_scale(db) - priv->iec_lower) /
		    (priv->iec_upper - priv->iec_lower);
    }

	cairo_set_source_rgba (cr, 0,0,0, 0.8); 

    if (priv->sides & GTK_METERSCALE_LEFT) {
	    cairo_rectangle (cr, pos, width/2 - mark/2, 1, mark+1);	
		cairo_fill (cr);
    }
    if (priv->sides & GTK_METERSCALE_RIGHT) {
		cairo_rectangle (cr, length -pos, width/2 - mark/2, 1, mark+1);
		cairo_fill (cr);
    }
    if (priv->sides & GTK_METERSCALE_TOP) {
		cairo_rectangle (cr, width/2 - mark/2, length - pos, mark +1, 1);
		cairo_fill (cr);	    
    }
    if (priv->sides & GTK_METERSCALE_BOTTOM) {
		cairo_rectangle (cr, width/2 - mark/2, pos, mark +1, 1);
		cairo_fill (cr);	    
    }
    

}

static void
gtk_meter_update (GtkMeter *meter)
{
  gfloat new_value;
  gboolean handled;
  GtkMeterPrivate *priv = meter->priv;
 // printf("in gtk_meter_update\n");
  
  g_return_if_fail (meter != NULL);
  g_return_if_fail (GTK_IS_METER (meter));

  new_value = gtk_adjustment_get_value(priv->adjustment);
  
  if (new_value < gtk_adjustment_get_lower(priv->adjustment))
    new_value = gtk_adjustment_get_lower(priv->adjustment);

  if (new_value > gtk_adjustment_get_upper(priv->adjustment))
    new_value = gtk_adjustment_get_upper(priv->adjustment);

  if (new_value != gtk_adjustment_get_value(priv->adjustment))
    {
      gtk_adjustment_set_value (priv->adjustment,new_value);
      
      g_signal_emit_by_name (G_OBJECT (priv->adjustment), "value_changed", &handled);

    }

  gtk_widget_queue_draw(GTK_WIDGET(meter)); 
}

static void
gtk_meter_adjustment_changed (GtkAdjustment *adjustment,
			      gpointer       data)
{
  GtkMeter *meter;
  GtkMeterPrivate *priv;
  
  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (data != NULL);

  meter = GTK_METER (data);
  priv = meter->priv;
  
  if ((priv->old_lower != gtk_adjustment_get_lower(adjustment)) ||
      (priv->old_upper != gtk_adjustment_get_upper(adjustment)))
    {
      priv->iec_lower = iec_scale(gtk_adjustment_get_lower(adjustment));
      priv->iec_upper = iec_scale(gtk_adjustment_get_upper(adjustment));

      gtk_meter_set_warn_point(meter, priv->amber_level);

      gtk_meter_update (meter);

      priv->old_value = gtk_adjustment_get_value(adjustment);
      priv->old_lower = gtk_adjustment_get_lower(adjustment);
      priv->old_upper = gtk_adjustment_get_upper(adjustment);
    } else if (priv->old_value != gtk_adjustment_get_value(adjustment)) {
      gtk_meter_update (meter);

      priv->old_value = gtk_adjustment_get_value(adjustment);
    }
}

static void
gtk_meter_adjustment_value_changed (GtkAdjustment *adjustment,
				    gpointer       data)
{
  GtkMeter *meter;
  GtkMeterPrivate *priv;
  
  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (data != NULL);

  meter = GTK_METER (data);
  priv = meter->priv;
  
  if (priv->old_value != gtk_adjustment_get_value(adjustment))
    {
      gtk_meter_update (meter);

      priv->old_value = gtk_adjustment_get_value(adjustment);
    }
}

static float iec_scale(float db)
{
    float def = 0.0f;		/* Meter deflection %age */

    if (db < -70.0f) {
	def = 0.0f;
    } else if (db < -60.0f) {
	def = (db + 70.0f) * 0.25f;
    } else if (db < -50.0f) {
	def = (db + 60.0f) * 0.5f + 5.0f;
    } else if (db < -40.0f) {
	def = (db + 50.0f) * 0.75f + 7.5;
    } else if (db < -30.0f) {
	def = (db + 40.0f) * 1.5f + 15.0f;
    } else if (db < -20.0f) {
	def = (db + 30.0f) * 2.0f + 30.0f;
    } else {
	def = (db + 20.0f) * 2.5f + 50.0f;
    }

    return def;
    
}

float gtk_meter_get_peak(GtkMeter *meter)
{
  GtkMeterPrivate *priv = meter->priv;
  return (priv->peak_db);
}

void gtk_meter_reset_peak(GtkMeter *meter)
{
	GtkMeterPrivate *priv = meter->priv;
    priv->peak = 0.0f;
    
}

void gtk_meter_set_warn_point(GtkMeter *meter, gfloat pt)
{
	GtkMeterPrivate *priv = meter->priv;
	
    priv->amber_level = pt;
    if (priv->direction == GTK_METER_LEFT || priv->direction ==
		    GTK_METER_DOWN) {
	priv->amber_frac = 1.0f - (iec_scale(priv->amber_level) -
		priv->iec_lower) / (priv->iec_upper - priv->iec_lower);
    } else {
        priv->amber_frac = (iec_scale(priv->amber_level) - priv->iec_lower) /
		(priv->iec_upper - priv->iec_lower);
    }

    gtk_widget_draw(GTK_WIDGET(meter), NULL);
}
