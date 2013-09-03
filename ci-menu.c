#include "ci-menu.h"
#include <memory.h>

enum CIMenuItemType {
    CIMenuItemTypeQuit = 0,
    CIMenuItemTypeShow,
    CIMenuItemTypeEditFont,
    CIMenuItemTypeEditFormat,
    CIMenuItemTypeEditMode,
    CIMenuItemTypeEditColor,
    CIMenuItemTypeSaveConfig,
    CIMenuItemTypeConnect,
    CIMenuItemTypeRefresh
};

struct CIMenuItem {
    enum CIMenuItemType type;
    gpointer data;
};

CIMenuItemCallbacks ci_menu_callbacks;
GSList *ci_menu_item_list = NULL;
CIPropertyGetFunc property_callback = NULL;

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

void ci_menu_init(CIPropertyGetFunc prop_cb, CIMenuItemCallbacks *callbacks)
{
    if (callbacks) {
        memcpy(&ci_menu_callbacks, callbacks, sizeof(CIMenuItemCallbacks));
    }
    property_callback = prop_cb;
}

void ci_menu_cleanup(void)
{
    ci_menu_reset();
}

void ci_menu_append_menu_item(GtkWidget *menu, const gchar *label, enum CIMenuItemType id, gpointer userdata)
{
    GtkWidget *item = gtk_menu_item_new_with_label(label);
    struct CIMenuItem *menu_item = ci_menu_item_new(id, userdata);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(ci_menu_handle), (gpointer)menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}

void ci_menu_append_check_menu_item(GtkWidget *menu, const gchar *label, enum CIMenuItemType id, gpointer userdata,
                                    gboolean checked)
{
    GtkWidget *item = gtk_check_menu_item_new_with_label(label);
    struct CIMenuItem *menu_item = ci_menu_item_new(id, userdata);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), checked);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(ci_menu_handle), (gpointer)menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}

void ci_menu_append_stock_menu_item(GtkWidget *menu, const gchar *stock_id, enum CIMenuItemType id, gpointer userdata)
{
    GtkWidget *item = gtk_image_menu_item_new_from_stock(stock_id, NULL);
    struct CIMenuItem *menu_item = ci_menu_item_new(id, userdata);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(ci_menu_handle), (gpointer)menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}

void ci_menu_append_separator(GtkWidget *menu)
{
    GtkWidget *item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}

GtkWidget *ci_menu_popup_menu(gpointer userdata)
{
    ci_menu_reset();
    GtkWidget *popup = gtk_menu_new();

    gboolean connected = FALSE;
    gboolean visible = FALSE;

    if (property_callback) {
        property_callback("client-connected", (gpointer)&connected);
        property_callback("window-visible", (gpointer)&visible);
    }

    ci_menu_append_menu_item(popup, visible ? "Hide" : "Show",
            CIMenuItemTypeShow, (gpointer)(gulong)(visible ? TRUE : FALSE));
    ci_menu_append_stock_menu_item(popup,
            connected ? GTK_STOCK_DISCONNECT : GTK_STOCK_CONNECT,
            CIMenuItemTypeConnect,
            (gpointer)(gulong)(connected ? TRUE : FALSE));
    ci_menu_append_separator(popup);
    ci_menu_append_stock_menu_item(popup, GTK_STOCK_QUIT, CIMenuItemTypeQuit, NULL);

    return popup;
}

GtkWidget *ci_menu_context_menu(gpointer userdata)
{
    ci_menu_reset();
    GtkWidget *popup = gtk_menu_new();

    gboolean edit_mode = FALSE;

    if (property_callback)
        property_callback("edit-mode", (gpointer)&edit_mode);

    ci_menu_append_menu_item(popup, "Refresh", CIMenuItemTypeRefresh, NULL);

    ci_menu_append_check_menu_item(popup, "Edit", CIMenuItemTypeEditMode,
                                   (gpointer)(gulong)(edit_mode ? TRUE : FALSE), edit_mode);
    if (edit_mode) {
        if (userdata) {
            ci_menu_append_separator(popup);
            ci_menu_append_menu_item(popup, "Edit Format", CIMenuItemTypeEditFormat, userdata);
            ci_menu_append_menu_item(popup, "Edit Font", CIMenuItemTypeEditFont, userdata);
            ci_menu_append_menu_item(popup, "Edit Color", CIMenuItemTypeEditColor, userdata);
        }
        ci_menu_append_separator(popup);
        ci_menu_append_menu_item(popup, "Edit Background Color", CIMenuItemTypeEditColor, NULL);

        ci_menu_append_separator(popup);
        ci_menu_append_menu_item(popup, "Save Config", CIMenuItemTypeSaveConfig, NULL);
    }

    ci_menu_append_separator(popup);
    ci_menu_append_stock_menu_item(popup, GTK_STOCK_QUIT, CIMenuItemTypeQuit, NULL);

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
                ci_menu_callbacks.handle_show(menu_item->data);
            break;
        case CIMenuItemTypeEditFont:
            if (ci_menu_callbacks.handle_edit_font)
                ci_menu_callbacks.handle_edit_font(menu_item->data);
            break;
        case CIMenuItemTypeEditFormat:
            if (ci_menu_callbacks.handle_edit_element)
                ci_menu_callbacks.handle_edit_element(menu_item->data);
            break;
        case CIMenuItemTypeEditMode:
            if (ci_menu_callbacks.handle_edit_mode)
                ci_menu_callbacks.handle_edit_mode(menu_item->data);
            break;
        case CIMenuItemTypeEditColor:
            if (ci_menu_callbacks.handle_edit_color)
                ci_menu_callbacks.handle_edit_color(menu_item->data);
            break;
        case CIMenuItemTypeSaveConfig:
            if (ci_menu_callbacks.handle_save_config)
                ci_menu_callbacks.handle_save_config();
            break;
        case CIMenuItemTypeConnect:
            if (ci_menu_callbacks.handle_connect)
                ci_menu_callbacks.handle_connect(menu_item->data);
            break;
        case CIMenuItemTypeRefresh:
            if (ci_menu_callbacks.handle_refresh)
                ci_menu_callbacks.handle_refresh();
            break;
        default:
            break;
    }
}
