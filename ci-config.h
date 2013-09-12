#ifndef __CI_CONFIG_H__
#define __CI_CONFIG_H__

#include <glib.h>

typedef enum {
    CIConfigTypeInvalid = 0,
    CIConfigTypeInt,
    CIConfigTypeUint,
    CIConfigTypeString,
    CIConfigTypeColor
} CIConfigType;

typedef struct CIConfigVariable CIConfigSetting;

void ci_config_add_setting(const gchar *group, const gchar *key,
                           CIConfigType type, gpointer default_value);
void ci_config_cleanup(void);
GList *ci_config_enum_settings(void); /* [element-type: gchar*, "group:key" */
CIConfigSetting *ci_config_get_setting(const gchar *key);
CIConfigType ci_config_setting_get_type(CIConfigSetting *setting);
gpointer ci_config_setting_get_value(CIConfigSetting *setting);
gpointer ci_conifg_setting_get_default_value(CIConfigSetting *setting);

gboolean ci_config_load(void);
gboolean ci_config_save(void);

gboolean ci_config_get(const gchar *key, gpointer value);
gboolean ci_config_set(const gchar *key, gpointer value);

#endif
