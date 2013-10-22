#include "ci-utils.h"
#include <string.h>
#include <glib/gprintf.h>

gchar *ci_util_format_string(const gchar *format, CIFormatCallback format_cb, gpointer userdata)
{
    if (format_cb == NULL)
        return g_strdup(format);

    gchar *fmt = g_strdup(format);
    gchar *str = NULL;
    GList *strlist = NULL;
    gsize i = 0;

    gchar *last = fmt;
    while (fmt[i] != 0) {
        if (fmt[i] == '%') {
            if (fmt[i+1] == '%') {
                fmt[i] = 0;
                strlist = g_list_prepend(strlist, (gpointer)last);
                strlist = g_list_prepend(strlist, (gpointer)"%");
                last = &fmt[i+2];
            }
            else {
                str = format_cb(fmt[i+1], userdata);
                if (str != NULL) {
                    fmt[i] = 0;
                    strlist = g_list_prepend(strlist, (gpointer)last);
                    strlist = g_list_prepend(strlist, (gpointer)str);
                    last = &fmt[i+2];
                }
            }
            i += 2;
        }
        else {
            ++i;
        }
    }
    if (last[0] != 0) {
        strlist = g_list_prepend(strlist, (gpointer)last);
    }

    strlist = g_list_reverse(strlist);
    gchar *result = ci_util_concat_str_list(strlist);

    g_list_free(strlist);
    g_free(fmt);

    return result;
}

gchar *ci_util_concat_str_list(GList *str_list)
{
    GList *tmp;
    gsize length = 0;
    gsize pos = 0;
    gchar *concat = NULL;

    tmp = str_list;
    while (tmp) {
        if (tmp->data)
            length += strlen((const gchar*)tmp->data);
        tmp = g_list_next(tmp);
    }

    concat = g_malloc(length+1);

    tmp = str_list;
    while (tmp) {
        if (tmp->data) {
            strcpy(&concat[pos], (const gchar*)tmp->data);
            pos += strlen((const gchar*)tmp->data);
        }
        tmp = g_list_next(tmp);
    }

    return concat;
}

GList *ci_util_list_truncate(GList *list, guint length, GFreeFunc freefunc)
{
    if (g_list_length(list) <= length)
        return list;
    GList *tmp = g_list_nth(list, length + 1);
    if (tmp->prev)
       tmp->prev->next = NULL;

    g_list_free_full(tmp, freefunc);

    return tmp;
}

gchar *ci_color_to_string(GdkRGBA *color)
{
    if (color == NULL)
        return NULL;

    gchar *string = g_malloc(8);
    g_sprintf(string, "#%02x%02x%02x",
            (guchar)(255*color->red) & 0xff,
            (guchar)(255*color->green) & 0xff,
            (guchar)(255*color->blue) & 0xff);

    return string;
}

void ci_string_to_color(GdkRGBA *color, const gchar *string)
{
    gdk_rgba_parse(color, string);
}

gchar *ci_util_strdup_clean_number(const gchar *number)
{
    gchar *result = g_strdup(number);
    return ci_util_clean_number(result);
}

gchar *ci_util_clean_number(gchar *number)
{
    if (number == NULL)
        return NULL;
    gsize i = 0, j = 0;
    while (number[i] != 0) {
        if (g_ascii_isdigit(number[i]))
            number[j++] = number[i++];
        else
            ++i;
    }
    number[j] = 0;

    return number;
}
