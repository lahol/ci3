#ifndef __CI_CONFIG_H__
#define __CI_CONFIG_H__

#include <glib.h>

typedef enum {
    CIConfigTypeInvalid = 0,
    CIConfigTypeInt,
    CIConfigTypeUint,
    CIConfigTypeString,
    CIConfigTypeColor,
    CIConfigTypeBoolean
} CIConfigType;

typedef struct CIConfigVariable CIConfigSetting;

void ci_config_add_setting(const gchar *group, const gchar *key,
                           CIConfigType type, gpointer default_value);
void ci_config_cleanup(void);
GList *ci_config_enum_settings(void); /* [element-type: gchar*, "group:key" */
CIConfigSetting *ci_config_get_setting(const gchar *key);
CIConfigType ci_config_setting_get_type(CIConfigSetting *setting);

gboolean ci_config_setting_get_value(CIConfigSetting *setting, gpointer value);
gboolean ci_conifg_setting_get_default_value(CIConfigSetting *setting, gpointer value);

gboolean ci_config_load(void);
gboolean ci_config_save(void);

gboolean ci_config_get(const gchar *key, gpointer value);
gboolean ci_config_set(const gchar *key, gpointer value);

/* Wrappers for convenience. */
gboolean ci_config_get_boolean(const gchar *key);
guint ci_config_get_uint(const gchar *key);
gint ci_config_get_int(const gchar *key);
gchar *ci_config_get_string(const gchar *key);
gchar *ci_config_get_color_as_string(const gchar *key);

#define ci_config_set_boolean(key, value) ci_config_set((key), GINT_TO_POINTER((value)))
#define ci_config_set_uint(key, value) ci_config_set((key), GUINT_TO_POINTER((value)))
#define ci_config_set_int(key, value) ci_config_set((key), GINT_TO_POINTER((value)))
#define ci_config_set_string(key, value) ci_config_set((key), (gpointer)(value))
gboolean ci_config_set_color_as_string(const gchar *key, gchar *value);

#endif
