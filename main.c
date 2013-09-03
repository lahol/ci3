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
#include "ci-call-list.h"
#include "ci-utils.h"
#include <memory.h>

void handle_quit(void)
{
    gtk_main_quit();
}

void handle_show(gpointer userdata)
{
    if (userdata)
        ci_window_hide();
    else
        ci_window_show(TRUE, FALSE);
}

gchar *ci_format_call_info(gchar conversion_symbol, CICallInfo *data)
{
    static char static_buffer[256];
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
        case 'i':
            static_buffer[0] = 0;
            if (data)
                sprintf(static_buffer, "%d", data->id);
            else
                return "<id>";
            return static_buffer;
        default:
            return NULL;
    }
    return NULL;
}

void msg_callback(CINetMsg *msg)
{
    g_printf("msg callback: guid: %u\n", msg->guid);
    if (msg->msgtype == CI_NET_MSG_VERSION) {
        g_printf("server version: %d.%d.%d (%s)\n", 
                ((CINetMsgVersion*)msg)->major,
                ((CINetMsgVersion*)msg)->minor,
                ((CINetMsgVersion*)msg)->patch,
                ((CINetMsgVersion*)msg)->human_readable);
    }
    else if (msg->msgtype == CI_NET_MSG_EVENT_RING) {
        ci_display_element_set_content_all((CIFormatCallback)ci_format_call_info,
                (gpointer)&((CINetMsgEventRing*)msg)->callinfo);
        ci_window_update();
        ci_window_show(((CINetMsgMultipart*)msg)->stage == MultipartStageInit ? TRUE : FALSE, FALSE);
    }
    else if (msg->msgtype == CI_NET_MSG_DB_NUM_CALLS) {
        g_printf("num calls: %d\n", ((CINetMsgDbNumCalls*)msg)->count);
    }
    else if (msg->msgtype == CI_NET_MSG_DB_CALL_LIST) {
        g_printf("msg call list\n");
    }
}

void handle_edit_mode(gpointer userdata)
{
    ci_window_set_mode((gboolean)(gulong)userdata ? CIWindowModeNormal : CIWindowModeEdit);
}

void handle_edit_element(gpointer userdata)
{
    if (userdata == NULL)
        return;
    CIDisplayContext *ctx = (CIDisplayContext*)userdata;
    char *format_string = NULL;
    if (ctx->type == CIDisplayContextDisplayElement && ctx->data[0] != NULL) {
        format_string = g_strdup(ci_display_element_get_format(ctx->data[0]));

        if (ci_window_edit_element_dialog(&format_string)) {
            ci_display_element_set_format(ctx->data[0], format_string);
            ci_display_element_set_content(ctx->data[0], (CIFormatCallback)ci_format_call_info, NULL);
            ci_window_update();
        }
        g_free(format_string);
    }
    else if (ctx->type == CIDisplayContextList) {
        CICallListColumn *column = ci_call_list_get_column(GPOINTER_TO_UINT(ctx->data[1]));
        if (column) {
            format_string = g_strdup(ci_call_list_get_column_format(column));

            if (ci_window_edit_element_dialog(&format_string)) {
                ci_call_list_set_column_format(column, format_string);
                ci_call_list_update_lines();
                ci_window_update();
            }

            g_free(format_string);
        }
    }

    g_free(ctx);
}

void handle_select_font(gpointer userdata)
{
    if (userdata == NULL)
        return;
    CIDisplayContext *ctx = (CIDisplayContext*)userdata;
    gchar *font = NULL;

    if (ctx->type == CIDisplayContextDisplayElement) {
        font = g_strdup(ci_display_element_get_font((CIDisplayElement*)ctx->data[0]));
        if (ci_window_select_font_dialog(&font)) {
            ci_display_element_set_font((CIDisplayElement*)ctx->data[0], font);
            g_free(font);
            ci_window_update();
        }
    }
    else if (ctx->type == CIDisplayContextList) {
        font = g_strdup(ci_call_list_get_font());
        if (ci_window_select_font_dialog(&font)) {
            ci_call_list_set_font(font);
            g_free(font);
            ci_window_update();
        }
    }

    g_free(ctx);
}

void handle_edit_color(gpointer userdata)
{
    if (userdata == NULL)
        return;
    CIDisplayContext *ctx = (CIDisplayContext*)userdata;
    GdkRGBA color;
    if (ci_window_choose_color_dialog(&color)) {
        if (ctx->type == CIDisplayContextDisplayElement)
            ci_display_element_set_color((CIDisplayElement*)ctx->data[0], &color);
        else if (ctx->type == CIDisplayContextNone)
            ci_window_set_background_color(&color);
        else if (ctx->type == CIDisplayContextList)
            ci_call_list_set_color(&color);
        ci_window_update();
    }
    g_free(ctx);
}

void handle_save_config(void)
{
    ci_config_save();
}

void handle_connect(gpointer userdata)
{
    if ((gboolean)(gulong)userdata) {
        client_stop();
    }
    else {
        client_connect();
    }
}

void update_list_query_callback(CINetMsg *msg, gpointer userdata);

void handle_refresh_list_reply(CINetMsg *msg, gpointer userdata)
{
    if (msg->msgtype != CI_NET_MSG_DB_CALL_LIST)
        return;

    GList *tmp = ((CINetMsgDbCallList*)msg)->calls;

    while (tmp) {
        g_print("[%03d]: %s %s %s %s (%s) %s %s\n",
                ((CICallInfo*)tmp->data)->id,
                ((CICallInfo*)tmp->data)->date,
                ((CICallInfo*)tmp->data)->time,
                ((CICallInfo*)tmp->data)->completenumber,
                ((CICallInfo*)tmp->data)->name,
                ((CICallInfo*)tmp->data)->areacode,
                ((CICallInfo*)tmp->data)->number,
                ((CICallInfo*)tmp->data)->area);
        tmp = g_list_next(tmp);
    }
}

void handle_refresh_query_reply(CINetMsg *msg, gpointer userdata)
{
    if (msg->msgtype != CI_NET_MSG_DB_NUM_CALLS)
        return;
    g_printf("handle refresh query reply: %d calls\n",
            ((CINetMsgDbNumCalls*)msg)->count);

    client_query(CIClientQueryCallList, handle_refresh_list_reply, NULL,
            "min-id", GINT_TO_POINTER(((CINetMsgDbNumCalls*)msg)->count-4),
            "count", GINT_TO_POINTER(4), "user", GINT_TO_POINTER(0), NULL, NULL);
}

void handle_refresh(void)
{
    client_query(CIClientQueryNumCalls, update_list_query_callback, NULL, NULL, NULL);
}

void update_list_query_callback(CINetMsg *msg, gpointer userdata)
{
    if (msg->msgtype == CI_NET_MSG_DB_NUM_CALLS) {
        ci_call_list_set_item_count(((CINetMsgDbNumCalls*)msg)->count);
        ci_call_list_set_offset(0);
    }
    else if (msg->msgtype == CI_NET_MSG_DB_CALL_LIST) {
        GList *tmp;
        guint index = 0;
        for (tmp = g_list_last(((CINetMsgDbCallList*)msg)->calls);
             tmp != NULL;
             tmp = g_list_previous(tmp), ++index) {
            ci_call_list_set_call(index, (CICallInfo*)tmp->data);
        }
    }
}

void handle_list_reload(gint offset, gint count)
{
    g_printf("reload: %d, %d\n", offset, count);
    client_query(CIClientQueryCallList, update_list_query_callback, NULL,
            "offset", GINT_TO_POINTER(offset),
            "count", GINT_TO_POINTER(count),
            "user", GINT_TO_POINTER(0),
            NULL, NULL);
}

void init_display(void)
{
    ci_display_element_set_content_all((CIFormatCallback)ci_format_call_info, NULL);
    ci_call_list_set_reload_func(handle_list_reload);
    ci_call_list_set_format_func((CIFormatCallback)ci_format_call_info);
    ci_call_list_set_line_count(6);
    CICallListColumn *col = ci_call_list_append_column();
    ci_call_list_set_column_format(col, "[%i] %D %T (%p) %n: %N");
    ci_call_list_set_column_width(col, 100);
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
        handle_save_config,
        handle_connect,
        handle_refresh
    };
    ci_menu_init(ci_property_get, &menu_cb);

    init_display();

    gtk_main();

    client_stop(); /* wait for reply */
    client_shutdown();
    ci_menu_cleanup();
    ci_display_element_clear_list();
    ci_call_list_cleanup();

    return 0;
}
