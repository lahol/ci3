#ifndef __ICON_H__
#define __ICON_H__

#include <gtk/gtk.h>

typedef GtkWidget *(*CIGetMenu)(gpointer);

gboolean ci_icon_create(CIGetMenu get_menu_cb, gpointer userdata);
void ci_icon_destroy(void);

#endif
