#ifndef __CI_DISPLAY_DATA_H__
#define __CI_DISPLAY_DATA_H__

#include <gtk/gtk.h>

typedef struct _CIDisplayElement CIDisplayElement;

CIDisplayElement *ci_display_element_new(void);
void ci_display_element_free(CIDisplayElement *element);
void ci_display_element_remove(CIDisplayElement *element);

GList *ci_display_element_get_elements(void);
CIDisplayElement *ci_display_element_get_from_pos(gdouble x, gdouble y);

void ci_display_element_clear_list(void);

void ci_display_element_set_pos(CIDisplayElement *element, gdouble x, gdouble y);
void ci_display_element_get_pos(CIDisplayElement *element, gdouble *x, gdouble *y);

void ci_display_element_set_maxwidth(CIDisplayElement *element, gdouble maxwidth);

void ci_display_element_set_action(CIDisplayElement *element, const gchar *action);
const gchar *ci_display_element_get_action(CIDisplayElement *element);

void ci_display_element_set_format(CIDisplayElement *element, const gchar *format);
const gchar *ci_display_element_get_format(CIDisplayElement *element);

/* conversion_symbol, userdata */
typedef gchar *(*CIDisplayElementFormatCallback)(gchar, gpointer);

void ci_display_element_set_content(CIDisplayElement *element,
        CIDisplayElementFormatCallback format_cb, gpointer userdata);
void ci_display_element_set_content_all(CIDisplayElementFormatCallback format_cb, gpointer userdata);
const gchar *ci_display_element_get_content(CIDisplayElement *element);

void ci_display_element_render(CIDisplayElement *element, cairo_t *cr);
void ci_display_element_render_all(cairo_t *cr);

#endif
