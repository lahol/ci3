#include "ci-client.h"
#include <gio/gio.h>
#include <glib/gprintf.h>
#include <cinet.h>
#include <memory.h>
#include "ci-config.h"

struct {
    GSocketClient *client;
    GSocketConnection *connection;
    gchar *host;
    guint port;
    CIMsgCallback callback;
    CIClientState state;
    GCancellable *cancel;
} ciclient;

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

        if (ciclient.callback)
            ciclient.callback(msg);
        cinet_msg_free(msg);
        return TRUE;
    }
    else if ((cond & G_IO_ERR) || (cond & G_IO_HUP)) {
        g_print("err || hup\n");
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
    ciclient.connection = g_socket_client_connect_to_host_finish(source, result, NULL);
    if (!ciclient.connection) {
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
}

void client_shutdown(void)
{
    g_print("client_shutdown\n");
/*    client_stop();*/
    if (ciclient.state == CIClientStateInitialized) {
        g_free(ciclient.host);
        ciclient.state = CIClientStateUnknown;
    }
}

CIClientState client_get_state(void)
{
    return ciclient.state;
}
