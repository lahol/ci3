#ifndef __CI_WINDOW_H__
#define __CI_WINDOW_H__

#include <gtk/gtk.h>

gboolean ci_window_init(gint x, gint y, gint w, gint h);
void ci_window_show(gboolean mark_as_urgent, gboolean focus);
void ci_window_update(void);
void ci_window_hide(void);
void ci_window_destroy(void);

gboolean ci_window_select_font_dialog(gpointer userdata);
gboolean ci_window_edit_element_dialog(gpointer userdata);

#endif
