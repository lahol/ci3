#ifndef __CI_UTILS_H__
#define __CI_UTILS_H__

#include <glib.h>
#include <gtk/gtk.h>
#include "gtk2-compat.h"

typedef gchar *(*CIFormatCallback)(gchar, gpointer);

gchar *ci_util_format_string(const gchar *format, CIFormatCallback format_cb, gpointer userdata);
gchar *ci_util_concat_str_list(GList *str_list);

GList *ci_util_list_truncate(GList *list, guint length, GFreeFunc freefunc);

gchar *ci_color_to_string(GdkRGBA *color);
void ci_string_to_color(GdkRGBA *color, const gchar *string);

#endif
