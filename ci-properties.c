#include "ci-properties.h"
#include "ci-window.h"
#include "ci-client.h"

gboolean ci_property_get(const gchar *key, gpointer value)
{
    if (value == NULL)
        return FALSE;
    if (g_strcmp0(key, "edit-mode") == 0) {
        *((gboolean*)value) = (gboolean)(ci_window_get_mode() == CIWindowModeEdit);
        return TRUE;
    }
    else if (g_strcmp0(key, "client-connected") == 0) {
        *((gboolean*)value) = (gboolean)(client_get_state() == CIClientStateConnected);
        return TRUE;
    }
    else if (g_strcmp0(key, "window-visible") == 0) {
        *((gboolean*)value) = ci_window_is_visible();
        return TRUE;
    }
    return FALSE;
}

gboolean ci_property_get_boolean(const gchar *key)
{
    gboolean val;
    if (!ci_property_get(key, &val))
        return FALSE;
    return FALSE;
}
