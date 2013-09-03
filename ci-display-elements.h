#ifndef __CI_DISPLAY_DATA_H__
#define __CI_DISPLAY_DATA_H__

#include <gtk/gtk.h>
#include "ci-utils.h"

typedef struct _CIDisplayElement CIDisplayElement;

CIDisplayElement *ci_display_element_new(void);
void ci_display_element_remove(CIDisplayElement *element);

GList *ci_display_element_get_elements(void);
CIDisplayElement *ci_display_element_get_from_pos(gdouble x, gdouble y);

void ci_display_element_clear_list(void);

void ci_display_element_drag_begin(CIDisplayElement *element);
void ci_display_element_drag_update(CIDisplayElement *element, gdouble dx, gdouble dy);
void ci_display_element_drag_finish(CIDisplayElement *element);

void ci_display_element_set_pos(CIDisplayElement *element, gdouble x, gdouble y);
void ci_display_element_get_pos(CIDisplayElement *element, gdouble *x, gdouble *y);

void ci_display_element_set_font(CIDisplayElement *element, const gchar *font);
gchar *ci_display_element_get_font(CIDisplayElement *element);

void ci_display_element_set_maxwidth(CIDisplayElement *element, gdouble maxwidth);

void ci_display_element_set_action(CIDisplayElement *element, const gchar *action);
const gchar *ci_display_element_get_action(CIDisplayElement *element);

void ci_display_element_set_format(CIDisplayElement *element, const gchar *format);
const gchar *ci_display_element_get_format(CIDisplayElement *element);

void ci_display_element_set_color(CIDisplayElement *element, GdkRGBA *color);
void ci_display_element_get_color(CIDisplayElement *element, GdkRGBA *color);

void ci_display_element_set_content(CIDisplayElement *element,
        CIFormatCallback format_cb, gpointer userdata);
void ci_display_element_set_content_all(CIFormatCallback format_cb, gpointer userdata);
const gchar *ci_display_element_get_content(CIDisplayElement *element);

void ci_display_element_render(CIDisplayElement *element, cairo_t *cr);
void ci_display_element_render_all(cairo_t *cr);

#endif
