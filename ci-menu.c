#include "ci-menu.h"
#include <memory.h>

enum CIMenuItemType {
    CIMenuItemTypeQuit = 0,
    CIMenuItemTypeShow,
    CIMenuItemTypeEditFont
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
    GtkWidget *item;
    struct CIMenuItem *menu_item;

    item = gtk_menu_item_new_with_label("Show");
    menu_item = ci_menu_item_new(CIMenuItemTypeShow, NULL);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(ci_menu_handle), (gpointer)menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(popup), item);

    item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
    menu_item = ci_menu_item_new(CIMenuItemTypeQuit, NULL);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(ci_menu_handle), (gpointer)menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(popup), item);

    return popup;
}

GtkWidget *ci_menu_context_menu(gpointer userdata)
{
    ci_menu_reset();
    GtkWidget *popup = gtk_menu_new();
    GtkWidget *item;
    struct CIMenuItem *menu_item;

    if (userdata) {
        item = gtk_menu_item_new_with_label("Edit Font");
        menu_item = ci_menu_item_new(CIMenuItemTypeEditFont, userdata);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(ci_menu_handle), (gpointer)menu_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(popup), item);
    }

    item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
    menu_item = ci_menu_item_new(CIMenuItemTypeQuit, NULL);
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
        case CIMenuItemTypeShow:
            if (ci_menu_callbacks.handle_show)
                ci_menu_callbacks.handle_show();
            break;
        case CIMenuItemTypeEditFont:
            if (ci_menu_callbacks.handle_edit_font)
                ci_menu_callbacks.handle_edit_font(menu_item->data);
            break;
        default:
            break;
    }
}
