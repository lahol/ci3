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
#include "gtk2-compat.h"
#include "ci-dialogs.h"

CICallInfo last_call;
gboolean preserve_last_caller_name = FALSE;
gboolean last_call_valid =  FALSE;

void handle_last_caller_info_reply(CINetMsg *msg, gpointer userdata);
void update_last_call_display(void);
void query_last_call_caller_info(void);

void handle_refresh(void);
void refresh_after_done(CINetMsg *msg, gpointer userdata)
{
    query_last_call_caller_info();
    ci_call_list_reload();
}

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
            if (data) return (data->number && data->number[0]) ? data->number : "<unknown>";
            return "<number>";
        case 'N':
            if (data) return (data->name && data->name[0]) ? data->name : "<unknown>";
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
            if (data) return (data->areacode && data->areacode[0]) ? data->areacode : "<unknown>";
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
    gchar *bkup_name = NULL;

    if (msg->msgtype == CI_NET_MSG_VERSION) {
        g_printf("server version: %d.%d.%d (%s)\n", 
                ((CINetMsgVersion*)msg)->major,
                ((CINetMsgVersion*)msg)->minor,
                ((CINetMsgVersion*)msg)->patch,
                ((CINetMsgVersion*)msg)->human_readable);
    }
    else if (msg->msgtype == CI_NET_MSG_EVENT_RING) {
        if (((CINetMsgMultipart*)msg)->stage != MultipartStageInit &&
                preserve_last_caller_name)
            bkup_name = g_strdup(last_call.name);
        else
            preserve_last_caller_name = FALSE;

        cinet_call_info_copy(&last_call, &((CINetMsgEventRing*)msg)->callinfo);
        last_call_valid = TRUE;

        if (bkup_name) {
            cinet_call_info_set_value(&last_call, "name", bkup_name);
            g_free(bkup_name);
        }

        update_last_call_display();
        ci_window_update();
        ci_window_show(((CINetMsgMultipart*)msg)->stage == MultipartStageInit ? TRUE : FALSE, FALSE);
        handle_refresh();

        if (((CINetMsgMultipart*)msg)->stage == MultipartStageInit &&
                last_call.completenumber != NULL &&
                last_call.completenumber[0] != 0)
           query_last_call_caller_info();
    }
    else if (msg->msgtype == CI_NET_MSG_DB_NUM_CALLS) {
        g_printf("num calls: %d\n", ((CINetMsgDbNumCalls*)msg)->count);
    }
    else if (msg->msgtype == CI_NET_MSG_DB_CALL_LIST) {
        g_printf("msg call list\n");
    }
}

void handle_last_caller_info_reply(CINetMsg *msg, gpointer userdata)
{
    if (msg->msgtype != CI_NET_MSG_DB_GET_CALLER)
        return;

    if (((CINetMsgDbGetCaller*)msg)->caller.name != NULL) {
        preserve_last_caller_name = TRUE;
        cinet_call_info_set_value(&last_call, "name", ((CINetMsgDbGetCaller*)msg)->caller.name);
        update_last_call_display();
        ci_window_update();
    }
}

void query_last_call_caller_info(void)
{
    gint user;
    ci_config_get("client:user", &user);
    client_query(CIClientQueryGetCaller,
            (CIQueryMsgCallback)handle_last_caller_info_reply, NULL,
            "user", GINT_TO_POINTER(user),
            "number", last_call.completenumber,
            NULL, NULL);
}

void update_last_call_display(void)
{
    ci_display_element_set_content_all((CIFormatCallback)ci_format_call_info,
            last_call_valid ? (gpointer)&last_call : NULL);
}

void handle_edit_mode(gpointer userdata)
{
    ci_window_set_mode((gboolean)(gulong)userdata ? CIWindowModeNormal : CIWindowModeEdit);
    ci_window_update();
}

void handle_edit_element(gpointer userdata)
{
    if (userdata == NULL)
        return;
    CIDisplayContext *ctx = (CIDisplayContext*)userdata;
    char *format_string = NULL;
    if (ctx->type == CIContextTypeDisplayElement && ctx->data[0] != NULL) {
        format_string = g_strdup(ci_display_element_get_format(ctx->data[0]));

        if (ci_window_edit_element_dialog(&format_string)) {
            ci_display_element_set_format(ctx->data[0], format_string);
            update_last_call_display();
            ci_window_update();
        }
        g_free(format_string);
    }
    else if (ctx->type == CIContextTypeList) {
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

void handle_add(gpointer userdata)
{
    g_printf("handle add\n");
    if (userdata == NULL)
        return;
    CIDisplayContext *ctx = (CIDisplayContext*)userdata;

    char *format_string = NULL;

    if (ctx->type == CIContextTypeNone) {
        CIDisplayElement *el = ci_display_element_new();

        if (ci_window_edit_element_dialog(&format_string)) {
            ci_display_element_set_format(el, format_string);
            g_printf("element set format: %s\n", format_string);
            ci_display_element_set_pos(el, (gdouble)GPOINTER_TO_INT(ctx->data[0]),
                                           (gdouble)GPOINTER_TO_INT(ctx->data[1]));
            update_last_call_display();
        }
        else {
            ci_display_element_remove(el);
        }
    }
    else if (ctx->type == CIContextTypeList) {
        CICallListColumn *col = ci_call_list_append_column();

        if (ci_window_edit_element_dialog(&format_string)) {
            ci_call_list_set_column_format(col, format_string);
            g_printf("column set format: %s\n", format_string);
            ci_call_list_update_lines();
        }
        else {
            ci_call_list_column_free(col);
        }
    }

    ci_window_update();
    g_free(ctx);
    g_free(format_string);
}

void handle_remove(gpointer userdata)
{
    if (userdata == NULL)
        return;

    CIDisplayContext *ctx = (CIDisplayContext*)userdata;

    if (ctx->type == CIContextTypeDisplayElement) {
        ci_display_element_remove(ctx->data[0]);
    }
    else if (ctx->type == CIContextTypeList) {
        ci_call_list_column_free_index(GPOINTER_TO_UINT(ctx->data[1]));
    }

    ci_window_update();
    g_free(ctx);
}

void handle_select_font(gpointer userdata)
{
    if (userdata == NULL)
        return;
    CIDisplayContext *ctx = (CIDisplayContext*)userdata;
    gchar *font = NULL;

    if (ctx->type == CIContextTypeDisplayElement) {
        font = g_strdup(ci_display_element_get_font((CIDisplayElement*)ctx->data[0]));
        if (ci_window_select_font_dialog(&font)) {
            ci_display_element_set_font((CIDisplayElement*)ctx->data[0], font);
            g_free(font);
            ci_window_update();
        }
    }
    else if (ctx->type == CIContextTypeList) {
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
        if (ctx->type == CIContextTypeDisplayElement)
            ci_display_element_set_color((CIDisplayElement*)ctx->data[0], &color);
        else if (ctx->type == CIContextTypeNone)
            ci_window_set_background_color(&color);
        else if (ctx->type == CIContextTypeList)
            ci_call_list_set_color(&color);
        ci_window_update();
    }
    g_free(ctx);
}

void handle_save_config(void)
{
    ci_config_save();
}

void handle_add_caller(gpointer userdata)
{
    g_print("add caller\n");
    CIDisplayContext *ctx = (CIDisplayContext*)userdata;

    CICallInfo *selection = NULL;
    gint user;

    ci_config_get("client:user", &user);

    if (ctx->type == CIContextTypeList) {
        selection = ci_call_list_get_call(GPOINTER_TO_UINT(ctx->data[0]));
    }
    else {
        /* current call */
        if (last_call.fields != 0) {
            /* last call valid */
            selection = &last_call;
        }
    }

    CICallerInfo ci;

    if (selection != NULL &&
            selection->completenumber != NULL &&
            selection->completenumber[0] != 0) {
        g_printf("add caller: %s %s\n", selection->completenumber, selection->name);
        cinet_caller_info_set_value(&ci, "number", selection->completenumber);
        cinet_caller_info_set_value(&ci, "name", selection->name);

        if (ci_dialogs_add_caller(&ci)) {
            g_print("query: %d, %s -> %s\n", user, ci.number, ci.name);
            client_query(CIClientQueryAddCaller, refresh_after_done, NULL,
                    "user", GINT_TO_POINTER(user),
                    "number", ci.number,
                    "name", ci.name,
                    NULL, NULL);
        }
    }
}

void handle_connect(gpointer userdata)
{
    if ((gboolean)(gulong)userdata) {
        client_disconnect();
    }
    else {
        client_connect();
    }
}

void update_list_query_callback(CINetMsg *msg, gpointer userdata);

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

    ci_window_update();
}

void handle_list_reload(gint offset, gint count)
{
    gint user;
    ci_config_get("client:user", &user);

    client_query(CIClientQueryCallList, update_list_query_callback, NULL,
            "offset", GINT_TO_POINTER(offset),
            "count", GINT_TO_POINTER(count),
            "user", GINT_TO_POINTER(user),
            NULL, NULL);
}

void handle_client_state_change(CIClientState state)
{
    if (state == CIClientStateConnected)
        handle_refresh();
}

void handle_about(void)
{
    ci_dialogs_about();
}

void init_display(void)
{
    ci_display_element_set_content_all((CIFormatCallback)ci_format_call_info, NULL);
    ci_call_list_set_format_func((CIFormatCallback)ci_format_call_info);
    ci_call_list_update_lines();
    ci_call_list_set_reload_func(handle_list_reload);
    ci_window_init();
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    if (!ci_config_load()) {
        g_printf("Failed to load configuration. Using defaults.\n");
    }
    
    client_set_state_changed_callback(handle_client_state_change);
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
        handle_refresh,
        handle_add,
        handle_remove,
        handle_add_caller,
        handle_about
    };
    ci_menu_init(ci_property_get, &menu_cb);

    init_display();

    gtk_main();

    client_disconnect(); /* wait for reply */
    client_shutdown();
    ci_menu_cleanup();
    ci_display_element_clear_list();
    ci_call_list_cleanup();

    return 0;
}
