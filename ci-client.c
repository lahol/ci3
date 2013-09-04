#include "ci-client.h"
#include <gio/gio.h>
#include <glib/gprintf.h>
#include <cinet.h>
#include <memory.h>
#include "ci-config.h"

struct CIClientQuery {
    guint32 guid;
    CIQueryMsgCallback callback;
    gpointer userdata;
};

struct {
    GSocketClient *client;
    GSocketConnection *connection;
    gchar *host;
    guint port;
    CIMsgCallback callback;
    CIClientStateChangedFunc state_changed_cb;
    CIClientState state;
    GCancellable *cancel;
    guint32 query_guid;
    GList *queries;
} ciclient;

guint32 client_gen_guid(void)
{
    /* TODO: check that next id not in queue */
    ++ciclient.query_guid;
    if (ciclient.query_guid == 0)
        ciclient.query_guid = 1;
    return ciclient.query_guid;
}

struct CIClientQuery *client_query_new(CIQueryMsgCallback callback, gpointer userdata)
{
    struct CIClientQuery *query = g_malloc(sizeof(struct CIClientQuery));
    query->guid = client_gen_guid();
    query->callback = callback;
    query->userdata = userdata;
    ciclient.queries = g_list_prepend(ciclient.queries, query);

    return query;
}

struct CIClientQuery *client_query_get(guint32 guid)
{
    GList *tmp = ciclient.queries;
    while (tmp) {
        if (((struct CIClientQuery*)tmp->data)->guid == guid)
            return ((struct CIClientQuery*)tmp->data);
        tmp = g_list_next(tmp);
    }
    return NULL;
}

void client_query_remove(struct CIClientQuery *query)
{
    ciclient.queries = g_list_remove(ciclient.queries, query);
    g_free(query);
}

/* return TRUE to stop further propagation */
gboolean client_handle_query_msg(CINetMsg *msg)
{
    if (msg == NULL)
        return TRUE;
    if (msg->guid == 0)
        return FALSE;
    struct CIClientQuery *query = client_query_get(msg->guid);
    if (query == NULL)
        return FALSE;

    if (query->callback)
        query->callback(msg, query->userdata);

    client_query_remove(query);

    return TRUE;
}

void client_send_message(GSocketConnection *conn, gchar *data, gsize len)
{
    GSocket *socket = g_socket_connection_get_socket(conn);
    if (!socket)
        return;
    if (!g_socket_is_connected(socket))
        return;
    gssize bytes = 0;
    gssize rc;
    while (bytes < len) {
        rc = g_socket_send(socket, &data[bytes], len-bytes, NULL, NULL);
        if (rc < 0) {
            return;
        }
        bytes += rc;
    }
}

gboolean client_incoming_data(GSocket *socket, GIOCondition cond, GSocketConnection *conn)
{
    gchar buf[32];
    CINetMsgHeader header;
    gssize bytes;
    gssize rc;
    gchar *msgdata = NULL;
    CINetMsg *msg = NULL;

    if (cond == G_IO_IN) {
        bytes = g_socket_receive(socket, buf, CINET_HEADER_LENGTH, NULL, NULL);
        if (bytes <= 0) {
            g_print("error reading\n");
            client_stop();
            return FALSE;
        }
        if (cinet_msg_read_header(&header, buf, bytes) < CINET_HEADER_LENGTH) {
            g_print("failed to read message header\n");
            return TRUE;
        }
        msgdata = g_malloc(CINET_HEADER_LENGTH + header.msglen);
        memcpy(msgdata, buf, bytes);
        bytes = 0;
        while (bytes < header.msglen) {
            rc = g_socket_receive(socket, &msgdata[CINET_HEADER_LENGTH + bytes],
                    header.msglen-bytes, NULL, NULL);
            if (rc <= 0) {
                g_print("error reading data\n");
                client_stop();
                return FALSE;
            }
            bytes += rc;
        }

        if (cinet_msg_read_msg(&msg, msgdata, CINET_HEADER_LENGTH + header.msglen) != 0) {
            g_print("error reading message\n");
            g_free(msgdata);
            return TRUE;
        }
        g_free(msgdata);

        if (msg->msgtype == CI_NET_MSG_LEAVE) {
            g_print("received leave reply\n");
            ciclient.state = CIClientStateInitialized;
            client_shutdown();
        }
        else if (msg->msgtype == CI_NET_MSG_SHUTDOWN) {
            g_print("server shutdown\n");
        }

        if (!client_handle_query_msg(msg) && ciclient.callback)
            ciclient.callback(msg);
        cinet_msg_free(msg);
        return TRUE;
    }
    else if ((cond & G_IO_ERR) || (cond & G_IO_HUP)) {
        g_print("err || hup\n");
        client_stop();
        return FALSE;
    }

    return TRUE;
}

void client_connected_func(GSocketClient *source, GAsyncResult *result, gpointer userdata)
{
    g_print("client_connected_func: cancel: %p\n", ciclient.cancel);
    if (ciclient.cancel && g_cancellable_is_cancelled(ciclient.cancel)) {
        g_print("client_connected_func: was cancelled\n");
        g_object_unref(ciclient.cancel);
        ciclient.cancel = NULL;
        return;
    }
    GError *err = NULL;
    ciclient.connection = g_socket_client_connect_to_host_finish(source, result, &err);
    if (!ciclient.connection) {
        g_printf("no connection: %s\n", err->message);
        g_error_free(err);
        return;
    }
    g_print("connection established\n");
    g_tcp_connection_set_graceful_disconnect(G_TCP_CONNECTION(ciclient.connection), TRUE);

    GSource *sock_source = NULL;
    GSocket *client_sock = g_socket_connection_get_socket(ciclient.connection);

    sock_source = g_socket_create_source(G_SOCKET(client_sock),
            G_IO_IN | G_IO_HUP | G_IO_ERR, NULL);
    if (sock_source) {
        g_source_set_callback(sock_source, (GSourceFunc)client_incoming_data,
                ciclient.connection, NULL);
        g_source_attach(sock_source, NULL);
    }

    ciclient.state = CIClientStateConnected;

    gchar *buffer = NULL;
    gsize len = 0;
    CINetMsg *msg = cinet_message_new(CI_NET_MSG_VERSION, 
            "major", 3, "minor", 0, "patch", 0, NULL, NULL);
    if (cinet_msg_write_msg(&buffer, &len, msg) == 0) {
        client_send_message(ciclient.connection, buffer, len);
        g_free(buffer);
    }
    cinet_msg_free(msg);

    g_object_unref(ciclient.cancel);
    ciclient.cancel = NULL;

    if (ciclient.state_changed_cb)
        ciclient.state_changed_cb(CIClientStateConnected);
}

void client_start(CIMsgCallback callback)
{
    g_print("client_start\n");
    gchar *host = NULL;
    guint port = 0;
    ci_config_get("client:host", (gpointer)&host);
    ci_config_get("client:port", (gpointer)&port);

    ciclient.client = g_socket_client_new();
    ciclient.host = g_strdup(host);
    ciclient.port = port;
    ciclient.callback = callback;
    ciclient.connection = NULL;

    ciclient.state = CIClientStateInitialized;

    client_connect();
}

void client_set_state_changed_callback(CIClientStateChangedFunc func)
{
    ciclient.state_changed_cb = func;
}

void client_connect(void) {
    g_print("client_connect\n");
    if (ciclient.state == CIClientStateConnecting || 
        ciclient.state == CIClientStateConnected) {
        client_stop();
    }
    ciclient.cancel = g_cancellable_new();

    g_socket_client_connect_to_host_async(ciclient.client,
            ciclient.host, ciclient.port, ciclient.cancel,
            (GAsyncReadyCallback)client_connected_func, NULL);
    ciclient.state = CIClientStateConnecting;
}

void client_stop(void)
{
    g_print("client_stop\n");
    gchar *buffer = NULL;
    gsize len;

    if (ciclient.state == CIClientStateConnecting) {
        g_print("cancel connecting operation\n");
        if (ciclient.cancel) {
            g_cancellable_cancel(ciclient.cancel);
            g_object_unref(ciclient.cancel);
        }
    }
    else if (ciclient.state == CIClientStateConnected) {
        g_print("disconnect\n");
        CINetMsg *msg = cinet_message_new(CI_NET_MSG_LEAVE, NULL, NULL);
        if (cinet_msg_write_msg(&buffer, &len, msg) == 0) {
            client_send_message(ciclient.connection, buffer, len);
            g_free(buffer);
        }
        cinet_msg_free(msg);

        g_print("shutdown socket\n");
        g_socket_shutdown(g_socket_connection_get_socket(ciclient.connection), TRUE, TRUE, NULL);
        g_object_unref(ciclient.connection);
        ciclient.connection = NULL;
    }

    g_print("end client_stop\n");
    ciclient.state = CIClientStateInitialized;

    if (ciclient.state_changed_cb)
        ciclient.state_changed_cb(CIClientStateDisconnected);
}

void client_shutdown(void)
{
    g_print("client_shutdown\n");
/*    client_stop();*/
    if (ciclient.state == CIClientStateInitialized) {
        g_free(ciclient.host);
        ciclient.state = CIClientStateUnknown;
        g_list_free_full(ciclient.queries, g_free);
    }
}

CIClientState client_get_state(void)
{
    return ciclient.state;
}

/* va_list args) { va_arg(args, type); }*/ 
void client_query_num_calls(CINetMsg **msg, guint32 guid, va_list ap)
{
    *msg = cinet_message_new(CI_NET_MSG_DB_NUM_CALLS, "guid", guid, NULL, NULL);
}

void client_query_call_list(CINetMsg **msg, guint32 guid, va_list ap)
{
    *msg = cinet_message_new(CI_NET_MSG_DB_CALL_LIST, "guid", guid, NULL, NULL);
    gchar *key;
    gpointer val;
    do {
        key = va_arg(ap, gchar*);
        val = va_arg(ap, gpointer);
        if (key) {
            cinet_message_set_value(*msg, key, val);
        }
    } while (key);
}

gboolean client_query(CIClientQueryType type, CIQueryMsgCallback callback, gpointer userdata, ...)
{
    va_list ap;
    gchar *msgdata = NULL;
    gsize msglen = 0;
    CINetMsg *msg = NULL;

    if (ciclient.state != CIClientStateConnected)
        return FALSE;

    struct CIClientQuery *query = client_query_new(callback, userdata);

    va_start(ap, userdata);
    switch (type) {
        case CIClientQueryNumCalls:
            client_query_num_calls(&msg, query->guid, ap);
            break;
        case CIClientQueryCallList:
            client_query_call_list(&msg, query->guid, ap);
            break;
        default:
            break;
    }
    va_end(ap);

    if (msg) {
        cinet_msg_write_msg(&msgdata, &msglen, msg);

        client_send_message(ciclient.connection, msgdata, msglen);

        g_free(msgdata);
        cinet_msg_free(msg);
    }

    return TRUE;
}
