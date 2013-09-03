#ifndef __CI_UTILS_H__
#define __CI_UTILS_H__

#include <glib.h>

typedef gchar *(*CIFormatCallback)(gchar, gpointer);

gchar *ci_util_format_string(const gchar *format, CIFormatCallback format_cb, gpointer userdata);
gchar *ci_util_concat_str_list(GList *str_list);

GList *ci_util_list_truncate(GList *list, guint length, GFreeFunc freefunc);

#endif
