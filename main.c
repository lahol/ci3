#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include "ci-logging.h"
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
#include "ci-filter.h"

#ifdef USELIBNOTIFY
#include "ci-notify.h"
#endif

CICallInfo last_call;
gboolean preserve_last_caller_name = FALSE;
gboolean last_call_valid =  FALSE;

void handle_last_caller_info_reply(CINetMsg *msg, gpointer userdata);
void update_last_call_display(void);
void query_last_call_caller_info(void);

void handle_refresh(void);

void refresh_display(void)
{
    query_last_call_caller_info();
    ci_call_list_reload();
}

void refresh_after_done(CINetMsg *msg, gpointer userdata)
{
    refresh_display();
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
            if (data) return (data->number && data->number[0]) ? data->number : _("<unknown>");
            return _("<number>");
        case 'N':
            if (data) return (data->name && data->name[0]) ? data->name : _("<unknown>");
            return _("<name>");
        case 'D':
            if (data) return data->date ? data->date : _("<unknown>");
            return _("<date>");
        case 'T':
            if (data) return data->time ? data->time : _("<unknown>");
            return _("<time>");
        case 'M':
            if (data) return data->msn ? data->msn : _("<unknown>");
            return _("<msn>");
        case 'A':
            if (data) return data->alias ? data->alias : _("<unknown>");
            return _("<alias>");
        case 'p':
            if (data) return (data->areacode && data->areacode[0]) ? data->areacode : _("<unknown>");
            return _("<areacode>");
        case 'P':
            if (data) return data->area ? data->area : _("<unknown>");
            return _("<area>");
        case 'i':
            static_buffer[0] = 0;
            if (data)
                sprintf(static_buffer, "%d", data->id);
            else
                return _("<id>");
            return static_buffer;
        default:
            return NULL;
    }
    return NULL;
}

void present(CINetMsgType type, CICallInfo *call_info, gchar *msgid)
{
    if (!ci_filter_msn_allowed(call_info->msn))
        return;

    gchar *output = ci_config_get_string("general:output");

    if (output == NULL || g_strcmp0(output, "default") == 0) {
        if (type == CI_NET_MSG_EVENT_RING)
            ci_window_show(FALSE, FALSE);
    }
#ifdef USELIBNOTIFY
    else if (g_strcmp0(output, "libnotify") == 0) {
        ci_notify_present(type, call_info, msgid);
    }
#endif
    else {
        LOG("Unknown output: \"%s\"\n", output);
    }
}

void msg_callback(CINetMsg *msg)
{
    DLOG("msg callback: guid: %u\n", msg->guid);
    gchar *bkup_name = NULL;
    CICallInfo ci;

    if (msg->msgtype == CI_NET_MSG_VERSION) {
        LOG("server version: %d.%d.%d (%s)\n", 
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

        present(msg->msgtype, &last_call, ((CINetMsgMultipart*)msg)->msgid);

        handle_refresh();

        if (((CINetMsgMultipart*)msg)->stage == MultipartStageInit &&
                last_call.completenumber != NULL &&
                last_call.completenumber[0] != 0)
           query_last_call_caller_info();
    }
    else if (msg->msgtype == CI_NET_MSG_EVENT_CALL) {
        memset(&ci, 0, sizeof(CICallInfo));
        cinet_call_info_copy(&ci, &((CINetMsgEventCall*)msg)->callinfo);

        present(msg->msgtype, &ci, NULL);
    }
    else if (msg->msgtype == CI_NET_MSG_DB_NUM_CALLS) {
        DLOG("num calls: %d\n", ((CINetMsgDbNumCalls*)msg)->count);
    }
    else if (msg->msgtype == CI_NET_MSG_DB_CALL_LIST) {
        DLOG("msg call list\n");
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
    gint user = ci_config_get_int("client:user");
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
    DLOG("handle add\n");
    if (userdata == NULL)
        return;
    CIDisplayContext *ctx = (CIDisplayContext*)userdata;

    char *format_string = NULL;

    if (ctx->type == CIContextTypeNone) {
        CIDisplayElement *el = ci_display_element_new();

        if (ci_window_edit_element_dialog(&format_string)) {
            ci_display_element_set_format(el, format_string);
            DLOG("element set format: %s\n", format_string);
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
            DLOG("column set format: %s\n", format_string);
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
    memset(&color, 0, sizeof(GdkRGBA));
    if (ctx->type == CIContextTypeDisplayElement)
        ci_display_element_get_color((CIDisplayElement*)ctx->data[0], &color);
    else if (ctx->type == CIContextTypeNone)
        ci_config_get("window:background", &color);
    else if (ctx->type == CIContextTypeList)
        ci_call_list_get_color(&color);

    if (ci_dialog_choose_color_dialog(NULL, &color)) {
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
    DLOG("add caller\n");
    CIDisplayContext *ctx = (CIDisplayContext*)userdata;

    CICallInfo *selection = NULL;
    gint user = ci_config_get_int("client:user");

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
    cinet_caller_info_init(&ci);

    if (selection != NULL &&
            selection->completenumber != NULL &&
            selection->completenumber[0] != 0) {
        DLOG("add caller: %s %s\n", selection->completenumber, selection->name);
        cinet_caller_info_set_value(&ci, "number", selection->completenumber);
        cinet_caller_info_set_value(&ci, "name", selection->name);

        if (ci_dialogs_add_caller(&ci)) {
            DLOG("query: %d, %s -> %s\n", user, ci.number, ci.name);
            client_query(CIClientQueryAddCaller, refresh_after_done, NULL,
                    "user", GINT_TO_POINTER(user),
                    "number", ci.number,
                    "name", ci.name,
                    NULL, NULL);
            cinet_caller_info_free(&ci);
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
    gint user = ci_config_get_int("client:user"); 
    client_query(CIClientQueryCallList, update_list_query_callback, NULL,
            "offset", GINT_TO_POINTER(offset),
            "count", GINT_TO_POINTER(count),
            "user", GINT_TO_POINTER(user),
            NULL, NULL);
}

void update_last_call_callback(CINetMsg *msg, gpointer userdata)
{
    DLOG("update_last_call_callback\n");
    if (msg->msgtype == CI_NET_MSG_DB_CALL_LIST) {
        if (((CINetMsgDbCallList*)msg)->calls == NULL)
            return;
        cinet_call_info_copy(&last_call,
                ((CICallInfo*)(((CINetMsgDbCallList*)msg)->calls->data)));
        last_call_valid = TRUE;
        update_last_call_display();
        ci_window_update();
    }
}

void set_last_call_from_list(void)
{
    DLOG("set_last_call_from_list\n");
    gint user = ci_config_get_int("client:user");
    client_query(CIClientQueryCallList, update_last_call_callback, NULL,
            "offset", GINT_TO_POINTER(0),
            "count", GINT_TO_POINTER(1),
            "user", GINT_TO_POINTER(user),
            NULL, NULL);
}

void handle_client_state_change(CIClientState state)
{
    DLOG("client state change\n");
    if (state == CIClientStateConnected) {
        handle_refresh();
        if (ci_config_get_boolean("general:show-on-connect")) {
            set_last_call_from_list();
            ci_window_show(FALSE, FALSE);
        }
    }
}

void handle_about(void)
{
    ci_dialogs_about();
}

void config_changed_cb(void)
{
    DLOG("config changed\n");
    ci_logging_reinit();
    client_restart(FALSE);
}

void handle_edit_config(void)
{
    ci_dialog_config_show(config_changed_cb);
}

void handle_phonebook(void)
{
    ci_dialog_phonebook_show(refresh_display);
}

void init_display(void)
{
    ci_display_element_set_content_all((CIFormatCallback)ci_format_call_info, NULL);
    ci_call_list_set_format_func((CIFormatCallback)ci_format_call_info);
    ci_call_list_update_lines();
    ci_call_list_set_reload_func(handle_list_reload);
    ci_window_init();
}

void init_config(void)
{
    ci_config_add_setting("general", "output", CIConfigTypeString, (gpointer)"default", TRUE);
    ci_config_add_setting("general", "msn-filter", CIConfigTypeString, NULL, TRUE);
    ci_config_add_setting("general", "log-file", CIConfigTypeString, NULL, TRUE);
    ci_config_add_setting("general", "show-on-connect", CIConfigTypeBoolean, GINT_TO_POINTER(FALSE), TRUE);

#ifdef USELIBNOTIFY
    ci_config_add_setting("libnotify", "timeout", CIConfigTypeInt, GINT_TO_POINTER(-1), TRUE);
#endif
    
    ci_config_add_setting("client", "host", CIConfigTypeString, (gpointer)"localhost", TRUE);
    ci_config_add_setting("client", "port", CIConfigTypeUint, GUINT_TO_POINTER(63690), TRUE);
    ci_config_add_setting("client", "retry-interval", CIConfigTypeInt, GINT_TO_POINTER(10), TRUE);
    ci_config_add_setting("client", "user", CIConfigTypeInt, GINT_TO_POINTER(0), TRUE);

    ci_config_add_setting("window", "x", CIConfigTypeInt, GINT_TO_POINTER(10), FALSE);
    ci_config_add_setting("window", "y", CIConfigTypeInt, GINT_TO_POINTER(60), FALSE);
    ci_config_add_setting("window", "width", CIConfigTypeInt, GINT_TO_POINTER(200), FALSE);
    ci_config_add_setting("window", "height", CIConfigTypeInt, GINT_TO_POINTER(100), FALSE);
    ci_config_add_setting("window", "set-urgency-hint", CIConfigTypeBoolean, GINT_TO_POINTER(FALSE), TRUE);

    GdkRGBA col = { 1.0, 1.0, 1.0, 1.0 };
    ci_config_add_setting("window", "background", CIConfigTypeColor, (gpointer)&col, TRUE);
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    init_config();

    if (!ci_config_load()) {
        LOG("Failed to load configuration. Using defaults.\n");
    }

    gchar *msnlist = ci_config_get_string("general:msn-filter");
    ci_filter_msn_set(msnlist);
    g_free(msnlist);
    
    client_set_state_changed_callback(handle_client_state_change);
    client_start(msg_callback);

    if (!ci_icon_create(ci_menu_popup_menu, NULL))
        LOG("failed to create icon\n");

    CIMenuItemCallbacks menu_cb = {
        .handle_quit         = handle_quit,
        .handle_show         = handle_show,
        .handle_edit_font    = handle_select_font,
        .handle_edit_element = handle_edit_element,
        .handle_edit_mode    = handle_edit_mode,
        .handle_edit_color   = handle_edit_color,
        .handle_save_config  = handle_save_config,
        .handle_connect      = handle_connect,
        .handle_refresh      = handle_refresh,
        .handle_add          = handle_add,
        .handle_remove       = handle_remove,
        .handle_add_caller   = handle_add_caller,
        .handle_about        = handle_about,
        .handle_edit_config  = handle_edit_config,
        .handle_phonebook    = handle_phonebook
    };
    ci_menu_init(ci_property_get, &menu_cb);

    init_display();

    gtk_main();

    client_disconnect(); /* wait for reply */
    client_shutdown();
    ci_menu_cleanup();
    ci_display_element_clear_list();
    ci_call_list_cleanup();
    ci_config_cleanup();
#ifdef USELIBNOTIFY
    ci_notify_cleanup();
#endif

    return 0;
}
