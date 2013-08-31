#ifndef __CI_PROPERTIES_H__
#define __CI_PROPERTIES_H__

#include <glib.h>

typedef gboolean (*CIPropertyGetFunc)(const gchar *, gpointer);

extern gboolean ci_property_get(const gchar *key, gpointer value);

#endif
