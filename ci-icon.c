#include "ci-icon.h"
#include "ci-window.h"

GtkStatusIcon *ci_status_icon = NULL;
CIGetMenu ci_get_menu_cb = NULL;
gpointer ci_get_menu_cb_data = NULL;

void ci_icon_popup_menu(GtkStatusIcon *icon, guint button, guint activate_time, gpointer userdata)
{
    if (!ci_get_menu_cb)
        return;

    GtkWidget *popup = ci_get_menu_cb(ci_get_menu_cb_data);

    gtk_widget_show_all(popup);
    gtk_menu_popup(GTK_MENU(popup), NULL, NULL, (GtkMenuPositionFunc)gtk_status_icon_position_menu,
            (gpointer)ci_status_icon, button, activate_time);
}

void ci_icon_activate(GtkStatusIcon *icon, gpointer userdata)
{
    ci_window_show(FALSE, TRUE);
}

gboolean ci_icon_create(CIGetMenu get_menu_cb, gpointer userdata)
{
#ifdef WIN32
/*#include "cilogo.xpm"
    GdkPixbuf *icon_pixbuf = gdk_pixbuf_new_from_xpm_data((const gchar**)cilogo);*/
#include "cilogo-pixbuf.h"
    GdkPixbuf *icon_pixbuf = gdk_pixbuf_new_from_inline(-1, cilogo, FALSE, NULL);

    ci_status_icon = gtk_status_icon_new_from_pixbuf(icon_pixbuf);
#else
    ci_status_icon = gtk_status_icon_new_from_file("cilogo.svg");
#endif
    if (ci_status_icon == NULL)
        return FALSE;

    gtk_status_icon_set_visible(ci_status_icon, TRUE);

    ci_get_menu_cb = get_menu_cb;
    ci_get_menu_cb_data = userdata;

    g_signal_connect(G_OBJECT(ci_status_icon), "popup-menu", G_CALLBACK(ci_icon_popup_menu), NULL);
    g_signal_connect(G_OBJECT(ci_status_icon), "activate", G_CALLBACK(ci_icon_activate), NULL);

    return TRUE;
}

void ci_icon_destroy(void)
{
}
