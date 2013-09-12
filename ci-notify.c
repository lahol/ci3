#include "ci-notify.h"
#include <libnotify/notify.h>
#include <string.h>
#include <glib/gprintf.h>
#include "ci-config.h"

struct CINotifyEvent {
    gchar msgid[16];
    NotifyNotification *notification;
};

GList *ci_notify_notifications = NULL;

struct {
    guint32 actions : 1;
    guint32 body : 1;
    guint32 body_markup : 1;
} server_caps;

gint ci_notify_event_compare_notification(struct CINotifyEvent *event1, NotifyNotification *notification);
gint ci_notify_event_compare_msgid(struct CINotifyEvent *event1, gchar *msgid);

/* if notification is not NULL use that as argument, else msgid */
struct CINotifyEvent *ci_notify_get_event(NotifyNotification *notification, gchar *msgid);
void ci_notify_remove_event(NotifyNotification *notification, gchar *msgid);

struct CINotifyEvent *ci_notify_event_new(gchar *msgid);
void ci_notify_event_free(struct CINotifyEvent *event);

void ci_notify_notification_closed(NotifyNotification *notification, gpointer userdata);

void ci_notify_init(void)
{
    notify_init(APPNAME);

    GList *caps = notify_get_server_caps();
    GList *cap;
    g_print("libnotify server caps:\n");
    for (cap = caps; cap != NULL; cap = g_list_next(cap)) {
        g_printf(" %s\n", (gchar *)cap->data);
        if (g_strcmp0((gchar *)cap->data, "body-markup") == 0)
            server_caps.body_markup = 1;
        else if (g_strcmp0((gchar *)cap->data, "body") == 0)
            server_caps.body = 1;
        else if (g_strcmp0((gchar *)cap->data, "actions") == 0)
            server_caps.actions = 1;
    }
    g_list_free_full(caps, g_free);
}

void ci_notify_present(CINetMsgType type, CICallInfo *call_info, gchar *msgid)
{
    if (!notify_is_initted())
        ci_notify_init();

    struct CINotifyEvent *event = ci_notify_get_event(NULL, msgid);
    if (event == NULL) {
        g_printf("make new event\n");
        event = ci_notify_event_new(msgid);
    }

    gchar *summary = NULL;
    gchar *body = NULL;

    if (type == CI_NET_MSG_EVENT_RING) {
        summary = g_strdup("New call");
        if (server_caps.body)
            body = g_markup_printf_escaped(server_caps.body_markup ?
                    "Call from <b>(%s) %s</b> to %s\n <b>%s</b> (%s)\n %s %s" :
                    "Call from (%s) %s to %s\n %s (%s)\n %s %s",
                    call_info->areacode, call_info->number, call_info->msn,
                    call_info->name, call_info->area,
                    call_info->time, call_info->date);
    }
    else if (type == CI_NET_MSG_EVENT_CALL) {
        summary = g_strdup("Outgoing call");
        if (server_caps.body)
            body = g_markup_printf_escaped(server_caps.body_markup ?
                    "Outgoing call to <b>%s</b>\n %s %s" : 
                    "Outgoing call to %s\n %s %s",
                    call_info->completenumber,
                    call_info->time, call_info->date);
    }
    else {
        ci_notify_remove_event(event->notification, event->msgid);
        return;
    }

    if (event->notification) {
        g_printf("update notification %s\n", msgid);
        notify_notification_update(event->notification, summary, body, NULL);
    }
    else {
        g_printf("new notification %s\n", msgid);
        event->notification = notify_notification_new(summary, body, NULL);
    }

    gint timeout = NOTIFY_EXPIRES_DEFAULT; /* default: -1, never: 0 */
    ci_config_get("libnotify:timeout", &timeout);

    notify_notification_set_timeout(event->notification, timeout > 0 ? timeout * 1000 : timeout);

    g_signal_connect(event->notification, "closed", G_CALLBACK(ci_notify_notification_closed), NULL);

    notify_notification_show(event->notification, NULL);
}

void ci_notify_cleanup(void)
{
    if (!notify_is_initted())
        return;

    g_list_free_full(ci_notify_notifications, (GDestroyNotify)ci_notify_event_free);
}

gint ci_notify_event_compare_notification(struct CINotifyEvent *event1, NotifyNotification *notification)
{
    return (gint)(event1->notification != notification);
}

gint ci_notify_event_compare_msgid(struct CINotifyEvent *event1, gchar *msgid)
{
    return g_strcmp0(event1->msgid, msgid);
}

/* if notification is not NULL use that as argument, else msgid */
GList *ci_notify_get_event_link(NotifyNotification *notification, gchar *msgid)
{
    GList *link;
    if (notification != NULL)
        link = g_list_find_custom(ci_notify_notifications, (gpointer)notification,
                (GCompareFunc)ci_notify_event_compare_notification);
    else
        link = g_list_find_custom(ci_notify_notifications, (gpointer)msgid,
                (GCompareFunc)ci_notify_event_compare_msgid);

    return link;
}

struct CINotifyEvent *ci_notify_get_event(NotifyNotification *notification, gchar *msgid)
{
    GList *result = ci_notify_get_event_link(notification, msgid);

    if (result == NULL)
        return NULL;

    return (struct CINotifyEvent *)result->data;
}

void ci_notify_remove_event(NotifyNotification *notification, gchar *msgid)
{
    GList *link = ci_notify_get_event_link(notification, msgid);

    if (link == NULL)
        return;

    ci_notify_event_free((struct CINotifyEvent *)link->data);
    ci_notify_notifications = g_list_delete_link(ci_notify_notifications, link);
}

struct CINotifyEvent *ci_notify_event_new(gchar *msgid)
{
    struct CINotifyEvent *event = g_malloc0(sizeof(struct CINotifyEvent));
    if (msgid != NULL)
        strncpy(event->msgid, msgid, 15);
    else
        event->msgid[0] = 0;

    ci_notify_notifications = g_list_prepend(ci_notify_notifications, event);

    return event;
}

void ci_notify_event_free(struct CINotifyEvent *event)
{
    if (event == NULL)
        return;
    if (event->notification != NULL)
        g_object_unref(G_OBJECT(event->notification));

    g_free(event);
}

void ci_notify_notification_closed(NotifyNotification *notification, gpointer userdata)
{
/*    gint reason = notify_notification_get_closed_reason(notification);*/
    /* https://people.gnome.org/~mccann/docs/notification-spec/notification-spec-latest.html#signal-notification-closed
     * Closed reason:
     *  1 – The notification expired.
     *  2 – The notification was dismissed by the user.
     *  3 – The notification was closed by a call to CloseNotification. (notify_notification_closed)
     *  4 – Undefined/reserved reasons.
     */

    g_printf("notification closed\n");
    ci_notify_remove_event(notification, NULL);
}
