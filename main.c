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
    client_stop();
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
    g_printf("received msg\n");
    if (msg->msgtype == CI_NET_MSG_VERSION) {
        g_printf("version:\n maj: %d\n min: %d\n pat: %d\n hum: %s\n",
                ((CINetMsgVersion*)msg)->major,
                ((CINetMsgVersion*)msg)->minor,
                ((CINetMsgVersion*)msg)->patch,
                ((CINetMsgVersion*)msg)->human_readable);
    }
    else if (msg->msgtype == CI_NET_MSG_LEAVE) {
        g_printf("received leaving reply\n");
        gtk_main_quit();
    }
    else if (msg->msgtype == CI_NET_MSG_EVENT_RING) {
        g_printf("msg ring\n");
        g_printf("stage: %d\n", ((CINetMsgMultipart*)msg)->stage);
        g_printf("part:  %d\n", ((CINetMsgMultipart*)msg)->part);
        g_printf("completenumber: %s\n", ((CINetMsgEventRing*)msg)->fields & CIF_COMPLETENUMBER ?
                ((CINetMsgEventRing*)msg)->completenumber : "(not set)");
        g_printf("areacode: %s\n", ((CINetMsgEventRing*)msg)->fields & CIF_AREACODE ?
                ((CINetMsgEventRing*)msg)->areacode : "(not set)");
        g_printf("number: %s\n", ((CINetMsgEventRing*)msg)->fields & CIF_NUMBER ?
                ((CINetMsgEventRing*)msg)->number : "(not set)");
        g_printf("date: %s\n", ((CINetMsgEventRing*)msg)->fields & CIF_DATE ?
                ((CINetMsgEventRing*)msg)->date : "(not set)");
        g_printf("time: %s\n", ((CINetMsgEventRing*)msg)->fields & CIF_TIME ?
                ((CINetMsgEventRing*)msg)->time : "(not set)");
        g_printf("msn: %s\n", ((CINetMsgEventRing*)msg)->fields & CIF_MSN ?
                ((CINetMsgEventRing*)msg)->msn : "(not set)");
        g_printf("alias: %s\n", ((CINetMsgEventRing*)msg)->fields & CIF_ALIAS ?
                ((CINetMsgEventRing*)msg)->alias : "(not set)");
        g_printf("area: %s\n", ((CINetMsgEventRing*)msg)->fields & CIF_AREA ?
                ((CINetMsgEventRing*)msg)->area : "(not set)");
        g_printf("name: %s\n", ((CINetMsgEventRing*)msg)->fields & CIF_NAME ?
                ((CINetMsgEventRing*)msg)->name : "(not set)");
        ci_display_element_set_content_all((CIDisplayElementFormatCallback)ci_format_entry_ring, (gpointer)msg);
        ci_window_update();
        ci_window_show(((CINetMsgMultipart*)msg)->stage == MultipartStageInit ? TRUE : FALSE, FALSE);
    }
    else if (msg->msgtype == CI_NET_MSG_SHUTDOWN) {
        g_printf("received shutdown message\n");
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
    g_printf("handle editmode: %p\n", userdata);
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

void init_display(void)
{
/*    CIDisplayElement *el;

    el = ci_display_element_new();
    ci_display_element_set_pos(el, 10, 10);
    ci_display_element_set_format(el, "%p %n %P");

    el = ci_display_element_new();
    ci_display_element_set_pos(el, 10, 30);
    ci_display_element_set_format(el, "%N an %M (%A)");

    el = ci_display_element_new();
    ci_display_element_set_pos(el, 10, 50);
    ci_display_element_set_format(el, "%D %T");

    el = ci_display_element_new();
    ci_display_element_set_pos(el, 10, 70);
    ci_display_element_set_format(el, "test\nnewline");
*/
    gint x, y, w, h;
    GdkRGBA col;
    ci_config_get("window:x", &x);
    ci_config_get("window:y", &y);
    ci_config_get("window:width", &w);
    ci_config_get("window:height", &h);
    ci_config_get("window:background", &col);

    ci_window_init(x, y, w, h);
    ci_window_set_background_color(&col);

    ci_display_element_set_content_all((CIDisplayElementFormatCallback)ci_format_entry, NULL);
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    if (!ci_config_load()) {
        g_printf("Failed to load configuration. Using defaults.\n");
    }

    gchar *host = NULL;
    guint port = 0;

    g_printf("host before: %p\n", host);

    ci_config_get("client:host", (gpointer)&host);
    ci_config_get("client:port", (gpointer)&port);

    g_printf("host after: %p (%s)\n", host, host);

    client_start(host, port, msg_callback);

    if (!ci_icon_create(ci_menu_popup_menu, NULL))
        g_printf("failed to create icon\n");

    CIMenuItemCallbacks menu_cb = {
        handle_quit,
        handle_show,
        handle_select_font,
        handle_edit_element,
        handle_edit_mode,
        handle_edit_color
    };
    ci_menu_init(ci_property_get, &menu_cb);

    init_display();

    gtk_main();

    client_shutdown();
    ci_menu_cleanup();
    ci_display_element_clear_list();

    return 0;
}
