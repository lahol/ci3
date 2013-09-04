#include "ci-display-elements.h"
#include <pango/pangocairo.h>
#include <glib/gprintf.h>
#include <string.h>
#include "ci-properties.h"

struct _CIDisplayElement {
    gdouble x;
    gdouble y;
    gdouble width;
    gdouble height;
    gdouble maxwidth;
    gchar *font;
    GdkRGBA color;
    gchar *format;
    gchar *content;
    gchar *action;
    guint32 flags;
    gdouble dx;
    gdouble dy;
};

enum CIDisplayElementFlag {
    CIDisplayElementFlagSelected = (1<<0),
    CIDisplayElementFlagDragged = (2<<0)
};

GList *ci_display_element_list = NULL;

gchar *ci_display_element_concat_str_list(GList *str_list);

CIDisplayElement *ci_display_element_new(void)
{
    CIDisplayElement *element = g_malloc0(sizeof(struct _CIDisplayElement));
    element->color.alpha = 1.0f;

    ci_display_element_list = g_list_prepend(ci_display_element_list, (gpointer)element);

    return element;
}

void ci_display_element_free(CIDisplayElement *element)
{
    if (element) {
        g_free(element->format);
        g_free(element->content);
        g_free(element->action);
        g_free(element->font);
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

void ci_display_element_drag_begin(CIDisplayElement *element)
{
    if (element == NULL)
        return;
    element->flags |= CIDisplayElementFlagDragged;
}

void ci_display_element_drag_update(CIDisplayElement *element, gdouble dx, gdouble dy)
{
    if (element == NULL || !(element->flags & CIDisplayElementFlagDragged))
        return;
    element->dx = dx;
    element->dy = dy;
}

void ci_display_element_drag_finish(CIDisplayElement *element)
{
    if (element == NULL || !(element->flags & CIDisplayElementFlagDragged))
        return;
    element->x += element->dx;
    element->y += element->dy;

    element->dx = 0.0;
    element->dy = 0.0;

    element->flags &= ~CIDisplayElementFlagDragged;
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

void ci_display_element_set_font(CIDisplayElement *element, const gchar *font)
{
    if (element) {
        g_free(element->font);
        element->font = g_strdup(font);
    }
}

gchar *ci_display_element_get_font(CIDisplayElement *element)
{
    if (element)
        return element->font;
    return NULL;
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

void ci_display_element_set_color(CIDisplayElement *element, GdkRGBA *color)
{
    if (element && color)
        memcpy(&element->color, color, sizeof(GdkRGBA));
}

void ci_display_element_get_color(CIDisplayElement *element, GdkRGBA *color)
{
    if (element && color)
        memcpy(color, &element->color, sizeof(GdkRGBA));
}

void ci_display_element_set_content(CIDisplayElement *element,
        CIFormatCallback format_cb, gpointer userdata)
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

    element->content = ci_util_format_string(element->format, format_cb, userdata);
}

void ci_display_element_set_content_all(CIFormatCallback format_cb, gpointer userdata)
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

void ci_display_element_render(CIDisplayElement *element, cairo_t *cr, gboolean outline)
{
    if (element == NULL)
        return;
    if (cr == NULL || element->content == NULL) {
        element->width = 0;
        element->height = 0;
        return;
    }
    PangoLayout *layout;
    PangoFontDescription *fdesc;
    int w, h;

    layout = pango_cairo_create_layout(cr);

    if (element->font)
        fdesc = pango_font_description_from_string(element->font);
    else
        fdesc = pango_font_description_from_string("Sans Bold 10");

    pango_layout_set_font_description(layout, fdesc);
    pango_font_description_free(fdesc);

    pango_layout_set_text(layout, element->content, -1);
    pango_layout_get_size(layout, &w, &h);

    element->width = ((gdouble)w)/PANGO_SCALE;
    element->height = ((gdouble)h)/PANGO_SCALE;

    cairo_identity_matrix(cr);
    cairo_translate(cr, element->x + element->dx,
                        element->y + element->dy);
    gdk_cairo_set_source_rgba(cr, &element->color);

    pango_cairo_update_layout(cr, layout);
    pango_cairo_show_layout(cr, layout);

    if (outline) {
        /* inverse of background? */
        /* cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); */
        cairo_rectangle(cr, 0.0, 0.0, element->width, element->height);
        gdouble dashes[] = { 1.0 };
        cairo_set_dash(cr, dashes, 1, 0);
        cairo_stroke(cr);
    }

    g_object_unref(layout);
}

void ci_display_element_render_all(cairo_t *cr)
{
    GList *tmp = ci_display_element_list;
    gboolean editmode;
    ci_property_get("edit-mode", &editmode);
    while (tmp) {
        ci_display_element_render((CIDisplayElement*)tmp->data, cr, editmode);
        tmp = g_list_next(tmp);
    }
}
