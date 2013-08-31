#ifndef __CI_WINDOW_H__
#define __CI_WINDOW_H__

#include <gtk/gtk.h>

typedef enum {
    CIWindowModeNormal,
    CIWindowModeEdit
} CIWindowMode;

gboolean ci_window_init(gint x, gint y, gint w, gint h);
void ci_window_show(gboolean mark_as_urgent, gboolean focus);
void ci_window_update(void);
void ci_window_hide(void);
void ci_window_destroy(void);

void ci_window_set_mode(CIWindowMode mode);
CIWindowMode ci_window_get_mode(void);

void ci_window_set_background_color(GdkRGBA *color);

gboolean ci_window_select_font_dialog(gpointer userdata);
gboolean ci_window_edit_element_dialog(gpointer userdata);
gboolean ci_window_choose_color_dialog(GdkRGBA *color);

#endif
