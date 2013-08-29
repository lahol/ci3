#include "ci-menu.h"
#include <memory.h>

enum CIMenuItemType {
    CIMenuItemTypeQuit = 0
};

struct CIMenuItem {
    enum CIMenuItemType type;
    gpointer data;
};

CIMenuItemCallbacks ci_menu_callbacks;
GSList *ci_menu_item_list = NULL;

void ci_menu_handle(GtkMenuItem *item, struct CIMenuItem *menu_item);

void ci_menu_reset(void)
{
    g_slist_free_full(ci_menu_item_list, g_free);
    ci_menu_item_list = NULL;
}

struct CIMenuItem *ci_menu_item_new(enum CIMenuItemType type, gpointer data)
{
    struct CIMenuItem *item = g_malloc(sizeof(struct CIMenuItem));
    item->type = type;
    item->data = data;

    ci_menu_item_list = g_slist_prepend(ci_menu_item_list, item);

    return item;
}

void ci_menu_init(CIMenuItemCallbacks *callbacks)
{
    if (callbacks) {
        memcpy(&ci_menu_callbacks, callbacks, sizeof(CIMenuItemCallbacks));
    }
}

void ci_menu_cleanup(void)
{
    ci_menu_reset();
}

GtkWidget *ci_menu_popup_menu(gpointer userdata)
{
    ci_menu_reset();
    GtkWidget *popup = gtk_menu_new();
    GtkWidget *item  = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
    struct CIMenuItem *menu_item = ci_menu_item_new(CIMenuItemTypeQuit, NULL);

    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(ci_menu_handle), (gpointer)menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(popup), item);

    return popup;
}

void ci_menu_handle(GtkMenuItem *item, struct CIMenuItem *menu_item)
{
    if (menu_item == NULL)
        return;

    switch (menu_item->type) {
        case CIMenuItemTypeQuit:
            if (ci_menu_callbacks.handle_quit)
                ci_menu_callbacks.handle_quit();
            break;
        default:
            break;
    }
}
