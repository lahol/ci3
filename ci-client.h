#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <glib.h>
#include <cinet.h>

typedef void (*CIMsgCallback)(CINetMsg *);

void client_start(gchar *host, guint port, CIMsgCallback callback);
void client_stop(void);
void client_shutdown(void);

#endif
