#include "ci-properties.h"
#include "ci-window.h"

gboolean ci_property_get(const gchar *key, gpointer value)
{
    if (value == NULL)
        return FALSE;
    if (g_strcmp0(key, "edit-mode") == 0) {
        *((gboolean*)value) = ci_window_get_mode() == CIWindowModeEdit ? TRUE : FALSE;
        return TRUE;
    }
    return FALSE;
}
