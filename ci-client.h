#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <glib.h>
#include <cinet.h>

typedef enum {
    CIClientStateUnknown = 0,
    CIClientStateInitialized,
    CIClientStateConnecting,
    CIClientStateConnected,
    CIClientStateDisconnected
} CIClientState;

typedef enum {
    CIClientQueryNumCalls,
    CIClientQueryCallList
} CIClientQueryType;

typedef void (*CIMsgCallback)(CINetMsg *);
typedef void (*CIClientStateChangedFunc)(CIClientState);

void client_start(CIMsgCallback callback);
void client_set_state_changed_callback(CIClientStateChangedFunc func);
void client_connect(void);
void client_stop(void);
void client_shutdown(void);

CIClientState client_get_state(void);

typedef void (*CIQueryMsgCallback)(CINetMsg *, gpointer);
gboolean client_query(CIClientQueryType type, CIQueryMsgCallback callback, gpointer userdata, ...);

#endif
