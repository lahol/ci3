#include "ci-display-elements.h"
#include <pango/pangocairo.h>
#include <glib/gprintf.h>
#include <string.h>

struct _CIDisplayElement {
    gdouble x;
    gdouble y;
    gdouble width;
    gdouble height;
    gdouble maxwidth;
    gchar *fontface;
    guint32 fontcolor;
    guint32 fontweigtht;
    guint32 fontflags; /* italic, underline, strikeout */
    gchar *format;
    gchar *content;
    gchar *action;
};

GList *ci_display_element_list = NULL;

gchar *ci_display_element_concat_str_list(GList *str_list);

CIDisplayElement *ci_display_element_new(void)
{
    CIDisplayElement *element = g_malloc0(sizeof(struct _CIDisplayElement));

    ci_display_element_list = g_list_prepend(ci_display_element_list, (gpointer)element);

    return element;
}

void ci_display_element_free(CIDisplayElement *element)
{
    if (element) {
        g_free(element->format);
        g_free(element->content);
        g_free(element->action);
        g_free(element->fontface);
        g_free(element);
    }
}

void ci_display_element_remove(CIDisplayElement *element)
{
    ci_display_element_list = g_list_remove(ci_display_element_list, (gpointer)element);
    ci_display_element_free(element);
}

GList *ci_display_element_get_elements(void)
{
    return ci_display_element_list;
}

CIDisplayElement *ci_display_element_get_from_pos(gdouble x, gdouble y)
{
    GList *tmp = ci_display_element_list;
    while (tmp) {
        if (((CIDisplayElement*)tmp->data)->x <= x &&
            ((CIDisplayElement*)tmp->data)->y <= y &&
            ((CIDisplayElement*)tmp->data)->x+((CIDisplayElement*)tmp->data)->width >= x &&
            ((CIDisplayElement*)tmp->data)->y+((CIDisplayElement*)tmp->data)->height >= y)
            return (CIDisplayElement*)tmp->data;
        tmp = g_list_next(tmp);
    }

    return NULL;
}

void ci_display_element_clear_list(void)
{
    g_list_free_full(ci_display_element_list, (GFreeFunc)ci_display_element_free);
    ci_display_element_list = NULL;
}

void ci_display_element_set_pos(CIDisplayElement *element, gdouble x, gdouble y)
{
    if (element) {
        element->x = x;
        element->y = y;
    }
}

void ci_display_element_get_pos(CIDisplayElement *element, gdouble *x, gdouble *y)
{
    if (element) {
        if (x) *x = element->x;
        if (y) *y = element->y;
    }
}

void ci_display_element_set_maxwidth(CIDisplayElement *element, gdouble maxwidth)
{
    if (element) {
        element->maxwidth = maxwidth;
    }
}

void ci_display_element_set_action(CIDisplayElement *element, const gchar *action)
{
    if (element) {
        g_free(element->action);
        if (action)
            element->action = g_strdup(action);
        else
            element->action = NULL;
    }
}

const gchar *ci_display_element_get_action(CIDisplayElement *element)
{
    if (element)
        return element->action;
    return NULL;
}

void ci_display_element_set_format(CIDisplayElement *element, const gchar *format)
{
    if (element) {
        g_free(element->format);
        if (format)
            element->format = g_strdup(format);
        else
            element->format = NULL;
    }
}

const gchar *ci_display_element_get_format(CIDisplayElement *element)
{
    if (element)
        return element->format;
    return NULL;
}

gchar *ci_display_element_concat_str_list(GList *str_list)
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

void ci_display_element_set_content(CIDisplayElement *element,
        CIDisplayElementFormatCallback format_cb, gpointer userdata)
{
    if (element == NULL)
        return;
    g_free(element->content);
    element->content = NULL;

    if (element->format == NULL) {
        return;
    }

    if (format_cb == NULL) {
        /* no format callback given, just copy string and return */
        element->content = g_strdup(element->format);
        return;
    }

    gchar *format = g_strdup(element->format);
    gchar *str = NULL;
    GList *strlist = NULL;
    gsize i = 0;

    gchar *last = format;
    while (format[i] != 0) {
        if (format[i] == '%') {
            if (format[i+1] == '%') {
                format[i] = 0;
                strlist = g_list_prepend(strlist, (gpointer)last);
                strlist = g_list_prepend(strlist, (gpointer)"%");
                last = &format[i+2];
            }
            else {
                str = format_cb(format[i+1], userdata);
                if (str != NULL) {
                    format[i] = 0;
                    strlist = g_list_prepend(strlist, (gpointer)last);
                    strlist = g_list_prepend(strlist, (gpointer)str);
                    last = &format[i+2];
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
    element->content = ci_display_element_concat_str_list(strlist);

    g_list_free(strlist);
    g_free(format);
}

void ci_display_element_set_content_all(CIDisplayElementFormatCallback format_cb, gpointer userdata)
{
    GList *tmp = ci_display_element_list;
    while (tmp) {
        ci_display_element_set_content((CIDisplayElement*)tmp->data, format_cb, userdata);
        tmp = g_list_next(tmp);
    }
}

const gchar *ci_display_element_get_content(CIDisplayElement *element)
{
    if (element)
        return element->content;
    return NULL;
}

void ci_display_element_render(CIDisplayElement *element, cairo_t *cr)
{
    if (element == NULL)
        return;
    if (cr == NULL || element->content == NULL) {
        element->width = 0;
        element->height = 0;
        return;
    }
    PangoLayout *layout;
    int w, h;

    layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, element->content, -1);
    pango_layout_get_size(layout, &w, &h);

    element->width = ((gdouble)w)/PANGO_SCALE;
    element->height = ((gdouble)h)/PANGO_SCALE;

    cairo_identity_matrix(cr);
    cairo_translate(cr, element->x, element->y);
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

    pango_cairo_update_layout(cr, layout);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);
}

void ci_display_element_render_all(cairo_t *cr)
{
    GList *tmp = ci_display_element_list;
    while (tmp) {
        ci_display_element_render((CIDisplayElement*)tmp->data, cr);
        tmp = g_list_next(tmp);
    }
}
