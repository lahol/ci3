#include "client.h"
#include <gio/gio.h>
#include <glib/gprintf.h>
#include <cinet.h>
#include <memory.h>

struct {
    GSocketClient *client;
    GSocketConnection *connection;
    gchar *host;
    guint port;
    CIMsgCallback callback;
} ciclient;

void client_send_message(GSocketConnection *conn, gchar *data, gsize len)
{
    g_printf("send message\n");
    GSocket *socket = g_socket_connection_get_socket(conn);
    if (!socket)
        return;
    g_printf("socket: %p\n", socket);
    if (!g_socket_is_connected(socket))
        return;
    g_print("socket is connected\n");
    gssize bytes = 0;
    gssize rc;
    while (bytes < len) {
        g_printf("written %d bytes\n", bytes);
        rc = g_socket_send(socket, &data[bytes], len-bytes, NULL, NULL);
        if (rc < 0) {
            g_print("error sending\n");
            return;
        }
        bytes += rc;
    }
    g_printf("done sending %zu bytes\n", bytes);
}

gboolean client_incoming_data(GSocket *socket, GIOCondition cond, GSocketConnection *conn)
{
    gchar buf[32];
    CINetMsgHeader header;
    gssize bytes;
    gssize rc;
    gchar *msgdata = NULL;
    CINetMsg *msg = NULL;

    switch (cond) {
        case G_IO_IN:
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

            if (ciclient.callback)
                ciclient.callback(msg);
            cinet_msg_free(msg);
            return TRUE;
        default:
            return TRUE;
    }
}

void client_connected_func(GSocketClient *source, GAsyncResult *result, gpointer userdata)
{
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

    gchar *buffer = NULL;
    gsize len = 0;
    CINetMsg *msg = cinet_message_new(CI_NET_MSG_VERSION, 
            "major", 3, "minor", 0, "patch", 0, NULL, NULL);
    if (cinet_msg_write_msg(&buffer, &len, msg) == 0) {
        client_send_message(ciclient.connection, buffer, len);
        g_free(buffer);
    }
    cinet_msg_free(msg);
}

void client_start(gchar *host, guint port, CIMsgCallback callback)
{
    ciclient.client = g_socket_client_new();
    ciclient.host = g_strdup(host);
    ciclient.port = port;
    ciclient.callback = callback;
    ciclient.connection = NULL;
    g_socket_client_connect_to_host_async(ciclient.client,
            host, port, NULL, (GAsyncReadyCallback)client_connected_func, NULL);
}

void client_stop(void)
{
    g_print("client_stop\n");
    gchar *buffer = NULL;
    gsize len;
    CINetMsg *msg = cinet_message_new(CI_NET_MSG_LEAVE, NULL, NULL);
    if (cinet_msg_write_msg(&buffer, &len, msg) == 0) {
        client_send_message(ciclient.connection, buffer, len);
        g_free(buffer);
    }
    cinet_msg_free(msg);
}

void client_shutdown(void)
{
    g_print("client_shutdown\n");
/*    client_stop();*/
    g_socket_shutdown(g_socket_connection_get_socket(ciclient.connection), TRUE, TRUE, NULL);
    g_free(ciclient.host);

    g_object_unref(ciclient.connection);
}
