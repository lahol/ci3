#ifndef __GTK2_COMPAT__
#define __GTK2_COMPAT__

#include <gtk/gtk.h>

#if !GTK_CHECK_VERSION(3,0,0)

typedef struct {
    gdouble red;
    gdouble green;
    gdouble blue;
    gdouble alpha;
} GdkRGBA;

#define GDK_KEY_Escape 0xff1b

#define gdk_cairo_set_source_rgba(cr,col) cairo_set_source_rgba((cr), (col)->red, (col)->green, (col)->blue, (col)->alpha)

int gtk_widget_get_allocated_height(GtkWidget *widget);
int gtk_widget_get_allocated_width(GtkWidget *widget);

gboolean gdk_rgba_parse(GdkRGBA *rgba, const gchar *spec);

#endif

#endif
