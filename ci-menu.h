#ifndef __CI_MENU_H__
#define __CI_MENU_H__

#include <gtk/gtk.h>

typedef struct {
    void (*handle_quit)(void);
    void (*handle_show)(void);
    void (*handle_edit_font)(gpointer data);
} CIMenuItemCallbacks;

void ci_menu_init(CIMenuItemCallbacks *callbacks);
void ci_menu_cleanup(void);

extern GtkWidget *ci_menu_popup_menu(gpointer userdata);
extern GtkWidget *ci_menu_context_menu(gpointer userdata);

#endif
