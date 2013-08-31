#include <glib.h>
#include <gio/gio.h>
#include <glib/gprintf.h>
#include "ci-client.h"
#include "ci-icon.h"
#include "ci-menu.h"
#include "ci-window.h"
#include "ci-display-elements.h"
#include "ci-data.h"
#include "ci-properties.h"
#include "ci-config.h"
#include <memory.h>

void handle_quit(void)
{
    gtk_main_quit();
}

void handle_show(void)
{
    ci_window_show(TRUE, FALSE);
}

gchar *ci_format_entry(gchar conversion_symbol, gpointer userdata)
{
    switch (conversion_symbol) {
        case 'n': return "<number>";
        case 'N': return "<name>";
        case 'D': return "<date>";
        case 'T': return "<time>";
        case 'M': return "<msn>";
        case 'A': return "<alias>";
        case 'p': return "<areacode>";
        case 'P': return "<area>";
        default: return NULL;
    }
    return NULL;
    
}

gchar *ci_format_entry_ring(gchar conversion_symbol, CINetMsgEventRing *data)
{
    switch (conversion_symbol) {
        case 'n':
            if (data) return data->number ? data->number : "<unknown>";
            return "<number>";
        case 'N':
            if (data) return data->name ? data->name : "<unknown>";
            return "<name>";
        case 'D':
            if (data) return data->date ? data->date : "<unknown>";
            return "<date>";
        case 'T':
            if (data) return data->time ? data->time : "<unknown>";
            return "<time>";
        case 'M':
            if (data) return data->msn ? data->msn : "<unknown>";
            return "<msn>";
        case 'A':
            if (data) return data->alias ? data->alias : "<unknown>";
            return "<alias>";
        case 'p':
            if (data) return data->areacode ? data->areacode : "<unknown>";
            return "<areacode>";
        case 'P':
            if (data) return data->area ? data->area : "<unknown>";
            return "<area>";
        default:
            return NULL;
    }
    return NULL;
}

void msg_callback(CINetMsg *msg)
{
    if (msg->msgtype == CI_NET_MSG_VERSION) {
        g_printf("server version: %d.%d.%d (%s)\n", 
                ((CINetMsgVersion*)msg)->major,
                ((CINetMsgVersion*)msg)->minor,
                ((CINetMsgVersion*)msg)->patch,
                ((CINetMsgVersion*)msg)->human_readable);
    }
    else if (msg->msgtype == CI_NET_MSG_EVENT_RING) {
        ci_display_element_set_content_all((CIDisplayElementFormatCallback)ci_format_entry_ring, (gpointer)msg);
        ci_window_update();
        ci_window_show(((CINetMsgMultipart*)msg)->stage == MultipartStageInit ? TRUE : FALSE, FALSE);
    }
}

void handle_edit_element(gpointer userdata)
{
    if (ci_window_edit_element_dialog(userdata)) {
        ci_display_element_set_content(userdata, (CIDisplayElementFormatCallback)ci_format_entry, NULL);
        ci_window_update();
    }
}

void handle_select_font(gpointer userdata)
{
    if (ci_window_select_font_dialog(userdata)) {
        ci_window_update();
    }
}

void handle_edit_mode(gpointer userdata)
{
    ci_window_set_mode((gboolean)(gulong)userdata ? CIWindowModeNormal : CIWindowModeEdit);
}

void handle_edit_color(gpointer userdata)
{
    GdkRGBA color;
    if (ci_window_choose_color_dialog(&color)) {
        if (userdata)
            ci_display_element_set_color((CIDisplayElement*)userdata, &color);
        else
            ci_window_set_background_color(&color);
        ci_window_update();
    }
}

void handle_save_config(void)
{
    ci_config_save();
}

void init_display(void)
{
    ci_display_element_set_content_all((CIDisplayElementFormatCallback)ci_format_entry, NULL);
    ci_window_init();
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    if (!ci_config_load()) {
        g_printf("Failed to load configuration. Using defaults.\n");
    }

    client_start(msg_callback);

    if (!ci_icon_create(ci_menu_popup_menu, NULL))
        g_printf("failed to create icon\n");

    CIMenuItemCallbacks menu_cb = {
        handle_quit,
        handle_show,
        handle_select_font,
        handle_edit_element,
        handle_edit_mode,
        handle_edit_color,
        handle_save_config
    };
    ci_menu_init(ci_property_get, &menu_cb);

    init_display();

    gtk_main();

    client_stop(); /* wait for reply */
    client_shutdown();
    ci_menu_cleanup();
    ci_display_element_clear_list();

    return 0;
}
