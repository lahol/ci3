#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <glib.h>
#include <cinet.h>

typedef enum {
    CIClientStateUnknown = 0,
    CIClientStateInitialized,
    CIClientStateConnecting,
    CIClientStateConnected
} CIClientState;

typedef enum {
    CIClientQueryNumCalls,
    CIClientQueryCallList
} CIClientQueryType;

typedef void (*CIMsgCallback)(CINetMsg *);

void client_start(CIMsgCallback callback);
void client_connect(void);
void client_stop(void);
void client_shutdown(void);

CIClientState client_get_state(void);

typedef void (*CIQueryMsgCallback)(CINetMsg *, gpointer);
void client_query(CIClientQueryType type, CIQueryMsgCallback callback, gpointer userdata, ...);

#endif
