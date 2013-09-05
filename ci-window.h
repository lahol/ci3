#ifndef __CI_WINDOW_H__
#define __CI_WINDOW_H__

#include <gtk/gtk.h>
#include "gtk2-compat.h"
#include "ci-menu.h"

typedef enum {
    CIWindowModeNormal,
    CIWindowModeEdit
} CIWindowMode;

typedef struct {
    CIContextType type;
    gpointer data[2];
} CIDisplayContext;

gboolean ci_window_init(void);
void ci_window_show(gboolean mark_as_urgent, gboolean focus);
void ci_window_update(void);
void ci_window_hide(void);
void ci_window_destroy(void);

void ci_window_set_mode(CIWindowMode mode);
CIWindowMode ci_window_get_mode(void);
gboolean ci_window_is_visible(void);

void ci_window_set_background_color(GdkRGBA *color);

gboolean ci_window_select_font_dialog(gchar **fontname);
gboolean ci_window_edit_element_dialog(gchar **format);
gboolean ci_window_choose_color_dialog(GdkRGBA *color);

#endif
