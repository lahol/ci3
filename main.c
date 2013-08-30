#include <glib.h>
#include <gio/gio.h>
#include <glib/gprintf.h>
#include "ci-client.h"
#include "ci-icon.h"
#include "ci-menu.h"
#include "ci-window.h"
#include "ci-display-elements.h"
#include "ci-data.h"
#include <memory.h>

void handle_quit(void)
{
    client_stop();
}

void handle_show(void)
{
    ci_window_show(TRUE, FALSE);
}

gchar *ci_format_entry(gchar conversion_symbol, CIDataSet *data)
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
    CIDataSet data;
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
        g_print("copy from ring event\n");
        memset(&data, 0, sizeof(CIDataSet));
        ci_data_set_from_ring_event(&data, (CINetMsgEventRing*)msg, FALSE);
        g_print("set content all\n");
        ci_display_element_set_content_all((CIDisplayElementFormatCallback)ci_format_entry, (gpointer)&data);
        g_print("update\n");
        ci_window_update();
        g_print("show\n");
        ci_window_show(((CINetMsgMultipart*)msg)->stage == MultipartStageInit ? TRUE : FALSE, FALSE);
        g_print("free\n");
        ci_data_set_free(&data);
        g_print("done\n");
    }
    else if (msg->msgtype == CI_NET_MSG_SHUTDOWN) {
        g_printf("received shutdown message\n");
    }
}

void init_display(void)
{
    CIDisplayElement *el;

    el = ci_display_element_new();

    ci_display_element_set_pos(el, 10, 10);
    ci_display_element_set_format(el, "%n %N %D %T %M %A %S %F %p %P");

    ci_display_element_set_content_all((CIDisplayElementFormatCallback)ci_format_entry, NULL);
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    client_start("localhost", 63690, msg_callback);

    if (!ci_icon_create(ci_menu_popup_menu, NULL))
        g_printf("failed to create icon\n");

    CIMenuItemCallbacks menu_cb = { handle_quit, handle_show };
    ci_menu_init(&menu_cb);
    ci_window_init(100, 100, 400, 200);

    init_display();

    gtk_main();

    client_shutdown();
    ci_menu_cleanup();
    ci_display_element_clear_list();

    return 0;
}
