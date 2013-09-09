#ifndef __CI_MENU_H__
#define __CI_MENU_H__

#include <gtk/gtk.h>
#include "ci-properties.h"

typedef struct {
    void (*handle_quit)(void);
    void (*handle_show)(gpointer data);
    void (*handle_edit_font)(gpointer data);
    void (*handle_edit_element)(gpointer data);
    void (*handle_edit_mode)(gpointer data);
    void (*handle_edit_color)(gpointer data);
    void (*handle_save_config)(void);
    void (*handle_connect)(gpointer data);
    void (*handle_refresh)(void);
    void (*handle_add)(gpointer data);
    void (*handle_remove)(gpointer data);
    void (*handle_add_caller)(gpointer data);
    void (*handle_about)(void);
} CIMenuItemCallbacks;

typedef enum {
    CIContextTypeNone,
    CIContextTypeDisplayElement,
    CIContextTypeList
} CIContextType;

void ci_menu_init(CIPropertyGetFunc prop_cb, CIMenuItemCallbacks *callbacks);
void ci_menu_cleanup(void);

extern GtkWidget *ci_menu_popup_menu(gpointer userdata);
extern GtkWidget *ci_menu_context_menu(CIContextType ctxtype, gpointer userdata);

#endif
