#ifndef __CI_CONFIG_H__
#define __CI_CONFIG_H__

#include <glib.h>

gboolean ci_config_load(void);
gboolean ci_config_save(void);

gboolean ci_config_get(const gchar *key, gpointer value);
gboolean ci_config_set(const gchar *key, gpointer value);

#endif
