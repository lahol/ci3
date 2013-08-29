#include <glib.h>
#include <gio/gio.h>
#include <glib/gprintf.h>
#include "ci-client.h"
#include "ci-icon.h"
#include "ci-menu.h"
#include <signal.h>
#include <memory.h>

void handle_quit(void)
{
    client_stop();
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
    }
    else if (msg->msgtype == CI_NET_MSG_SHUTDOWN) {
        g_printf("received shutdown message\n");
    }
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    client_start("localhost", 63690, msg_callback);

    if (!ci_icon_create(ci_menu_popup_menu, NULL))
        g_printf("failed to create icon\n");

    CIMenuItemCallbacks menu_cb = { handle_quit };
    ci_menu_init(&menu_cb);

    gtk_main();

    client_shutdown();
    ci_menu_cleanup();

    return 0;
}
