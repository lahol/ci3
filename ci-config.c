#include "ci-config.h"
#include <json-glib/json-glib.h>
#include "ci-logging.h"
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include "ci-display-elements.h"
#include "ci-call-list.h"
#include <memory.h>
#include "gtk2-compat.h"

struct CIConfigVariable {
    CIConfigType type;
    gchar *key;
    gpointer default_value;
    gpointer value;
};

struct CIConfigGroup {
    gchar *groupname;
    GList *variables; /* [element-type: struct CIConfigVariable] */
};

GList *ci_config_groups = NULL; /* [element-type: struct CIConfigGroup] */

gchar *ci_color_to_string(GdkRGBA *color)
{
    if (color == NULL)
        return NULL;

    gchar *string = g_malloc(8);
    g_sprintf(string, "#%02x%02x%02x",
            (guchar)(255*color->red) & 0xff,
            (guchar)(255*color->green) & 0xff,
            (guchar)(255*color->blue) & 0xff);

    return string;
}

void ci_string_to_color(GdkRGBA *color, const gchar *string)
{
    gdk_rgba_parse(color, string);
}

gchar *ci_config_get_config_file(void)
{
    return g_build_filename(
            g_get_user_config_dir(),
            "ciclient.conf",
            NULL);
}

void ci_config_set_defaults(void)
{
    GList *group, *var;
    struct CIConfigVariable *setting;

    for (group = ci_config_groups; group != NULL; group = g_list_next(group)) {
        if (group->data == NULL)
            continue;
        for (var = (GList*)group->data; var != NULL; var = g_list_next(var)) {
            if (var->data == NULL)
                continue;
            setting = (struct CIConfigVariable*)var->data;
            switch (setting->type) {
                case CIConfigTypeInt:
                case CIConfigTypeUint:
                    setting->value = setting->default_value;
                    break;
                case CIConfigTypeString:
                case CIConfigTypeColor:
                    if (setting->value != setting->default_value)
                        g_free(setting->value);
                    setting->value = setting->default_value;
                    break;
                default:
                    break;
            }
        }
    }
}

void ci_config_value_clear(struct CIConfigVariable *value)
{
    if (value->type == CIConfigTypeString ||
            value->type == CIConfigTypeColor) {
        g_free(value->default_value);
        if (value->value != value->default_value)
            g_free(value->value);
    }
    value->value = NULL;
    value->default_value = NULL;
}

struct CIConfigGroup *ci_config_get_group(const gchar *name, gboolean create)
{
    if (name == NULL || name[0] == 0)
        return NULL;
    GList *group;

    for (group = ci_config_groups; group != NULL; group = g_list_next(group)) {
        if (g_strcmp0(((struct CIConfigGroup*)group->data)->groupname, name) == 0)
            return (struct CIConfigGroup*)group->data;
    }

    if (create) {
        struct CIConfigGroup *data = g_malloc0(sizeof(struct CIConfigGroup));

        data->groupname = g_strdup(name);

        ci_config_groups = g_list_prepend(ci_config_groups, data);

        return data;
    }
    else
        return NULL;
}

struct CIConfigVariable *ci_config_get_variable(const gchar *group, const gchar *name, gboolean create)
{
    if (name == NULL || name[0] == 0)
        return NULL;

    struct CIConfigGroup *grp = ci_config_get_group(group, create);
    if (grp == NULL)
        return NULL;

    GList *var;
    for (var = grp->variables; var != NULL; var = g_list_next(var)) {
        if (g_strcmp0(((struct CIConfigVariable*)var->data)->key, name) == 0)
            return (struct CIConfigVariable*)var->data;
    }

    if (create) {
        struct CIConfigVariable *data = g_malloc0(sizeof(struct CIConfigVariable));

        data->key = g_strdup(name);

        grp->variables = g_list_prepend(grp->variables, data);

        return data;
    }
    else
        return NULL;
}

void ci_config_add_setting(const gchar *group, const gchar *key,
                           CIConfigType type, gpointer default_value)
{
    struct CIConfigVariable *setting = ci_config_get_variable(group, key, TRUE);
    if (setting == NULL)
        return;

    ci_config_value_clear(setting);

    setting->type = type;
    if (type == CIConfigTypeString)
        setting->default_value = (gpointer)g_strdup((gchar *)default_value);
    else if (type == CIConfigTypeColor) {
        setting->default_value = g_malloc(sizeof(GdkRGBA));
        memcpy(setting->default_value, default_value, sizeof(GdkRGBA));
    }
    else
        setting->default_value = default_value;

    /* CAUTION: if value == default_value first alloc value */
    setting->value = setting->default_value;
}

void ci_config_value_free(gpointer value)
{
    struct CIConfigVariable *var = (struct CIConfigVariable*)value;

    if (var == NULL)
        return;

    ci_config_value_clear(var);
    g_free(var->key);
    g_free(var);
}

void ci_config_cleanup(void)
{
    GList *groups = NULL;

    for (groups = ci_config_groups; groups != NULL; groups = g_list_next(groups)) {
        g_list_free_full((GList*)((struct CIConfigGroup*)groups->data)->variables,
                (GDestroyNotify)ci_config_value_free);
    }

    g_list_free(ci_config_groups);
    ci_config_groups = NULL;
}

gboolean ci_config_load_root(JsonNode *node);
gboolean ci_config_load_settings(JsonNode *node);
gboolean ci_config_load_group(JsonNode *node, struct CIConfigGroup *group);
gboolean ci_config_load_element_array(JsonNode *node);
gboolean ci_config_load_element(JsonNode *node);
gboolean ci_config_load_list(JsonNode *node);
gboolean ci_config_load_list_column_array(JsonNode *node);
gboolean ci_config_load_list_column(JsonNode *node);

JsonNode *ci_config_save_root(void);
void ci_config_save_settings(JsonBuilder *builder);
void ci_config_save_group(JsonBuilder *builder, struct CIConfigGroup *group);
void ci_config_save_element_array(JsonBuilder *builder);
void ci_config_save_element(JsonBuilder *builder, CIDisplayElement *element);
void ci_config_save_list(JsonBuilder *builder);
void ci_config_save_list_column_array(JsonBuilder *builder);
void ci_config_save_list_column(JsonBuilder *builder, CICallListColumn *column);

gboolean ci_config_load(void)
{
    gboolean result = TRUE;
    gchar *filename = ci_config_get_config_file();
    JsonParser *parser = json_parser_new();
    JsonNode *root;
    GError *err = NULL;

    ci_config_set_defaults();

    if (!json_parser_load_from_file(parser, filename, &err)) {
        LOG("load from file failed: %s\n", err->message);
        g_error_free(err);
        result = FALSE;
    }
    else {
        root = json_parser_get_root(parser);
        result = ci_config_load_root(root);
    }

    g_object_unref(parser);
    g_free(filename);
    
    if (result == FALSE)
        ci_config_set_defaults();
    return result;
}

gboolean ci_config_save(void)
{
    gboolean result = TRUE;
    GError *err = NULL;
    gchar *filename = ci_config_get_config_file();
    JsonGenerator *gen = json_generator_new();
    JsonNode *root = ci_config_save_root();
    json_generator_set_root(gen, root);

    json_generator_set_pretty(gen, TRUE);
    json_generator_set_indent_char(gen, ' ');
    json_generator_set_indent(gen, 4);

    if (!json_generator_to_file(gen, filename, &err)) {
        LOG("save to file failed: %s\n", err->message);
        g_error_free(err);
        result = FALSE;
    }

    json_node_free(root);
    g_object_unref(gen);
    g_free(filename);

    return result;
}

gboolean ci_config_variable_get(struct CIConfigVariable *var, gpointer value, gboolean get_default)
{
    switch (var->type) {
        case CIConfigTypeInt:
            *((gint *)value) = GPOINTER_TO_INT((get_default ? var->default_value : var->value));
            break;
        case CIConfigTypeUint:
            *((guint *)value) = GPOINTER_TO_UINT((get_default ? var->default_value : var->value));
            break;
        case CIConfigTypeString:
            *((gchar **)value) = g_strdup((gchar *)(get_default ? var->default_value : var->value));
            break;
        case CIConfigTypeColor:
            memcpy(value, (get_default ? var->default_value : var->value), sizeof(GdkRGBA));
            break;
        default:
            return FALSE;
    }

    return TRUE;
}

gboolean ci_config_get(const gchar *key, gpointer value)
{
    /* Although this is somewhat inconsequent, this is a setting which is
     * never saved or accessed via the standard interface. */
    if (g_strcmp0(key, "config-file") == 0) {
        *((gchar **)value) = ci_config_get_config_file();
        return TRUE;
    }

    /* from now on we expect a string of the form "group:value" where both parts are not empty */
    gchar **split = g_strsplit(key, ":", 2);
    if (split[0] == NULL || split[1] == NULL)
        goto out;

    struct CIConfigVariable *setting = ci_config_get_variable(split[0], split[1], FALSE);
    if (setting == NULL)
        goto out;

    g_strfreev(split);

    return ci_config_variable_get(setting, value, FALSE);

out:
    g_strfreev(split);
    return FALSE;
}

gboolean ci_config_variable_set(struct CIConfigVariable *var, gpointer value)
{
    if (var == NULL)
        return FALSE;

    switch (var->type) {
        case CIConfigTypeInt:
        case CIConfigTypeUint:
            var->value = value;
            break;
        case CIConfigTypeString:
            if (var->value != var->default_value)
                g_free(var->value);
            var->value = (gpointer)g_strdup((gchar *)value);
            break;
        case CIConfigTypeColor:
            if (var->value == var->default_value) {
                var->value = g_malloc(sizeof(GdkRGBA));
            }
            memcpy(var->value, value, sizeof(GdkRGBA));
            break;
        default:
            return FALSE;
    }
    
    return TRUE;
}

gboolean ci_config_set(const gchar *key, gpointer value)
{
    gchar **split = g_strsplit(key, ":", 2);
    if (split[0] == NULL || split[1] == NULL)
        goto out;

    struct CIConfigVariable *setting = ci_config_get_variable(split[0], split[1], FALSE);
    if (setting == NULL)
        goto out;

    g_strfreev(split);

    return ci_config_variable_set(setting, value);
out:
    g_strfreev(split);
    return FALSE;
}

/* [element-type: gchar*, "group:key" */
GList *ci_config_enum_settings(void)
{
    GList *settings = NULL;
    GList *group, *var;
    for (group = ci_config_groups; group != NULL; group = g_list_next(group)) {
        for (var = (GList*)((struct CIConfigGroup*)group->data)->variables;
                var != NULL; var = g_list_next(var)) {
            settings = g_list_insert_sorted(settings,
                    g_strjoin(":",
                        ((struct CIConfigGroup*)group->data)->groupname,
                        ((struct CIConfigVariable*)var->data)->key,
                        NULL), (GCompareFunc)g_strcmp0);
        }
    }

    return settings;
}

CIConfigSetting *ci_config_get_setting(const gchar *key)
{
    gchar **split = g_strsplit(key, ":", 2);
    if (split[0] == NULL || split[1] == NULL) {
        g_strfreev(split);
        return NULL;
    }

    struct CIConfigVariable *result = ci_config_get_variable(split[0], split[1], FALSE);

    g_strfreev(split);

    return result;
}

CIConfigType ci_config_setting_get_type(CIConfigSetting *setting)
{
    if (setting == NULL)
        return CIConfigTypeInvalid;
    return setting->type;
}

gboolean ci_config_setting_get_value(CIConfigSetting *setting, gpointer value)
{
    return ci_config_variable_get(setting, value, FALSE);
}

gboolean ci_conifg_setting_get_default_value(CIConfigSetting *setting, gpointer value)
{
    return ci_config_variable_get(setting, value, TRUE);
}

gboolean ci_config_load_root(JsonNode *node)
{
    if (!JSON_NODE_HOLDS_OBJECT(node))
        return FALSE;
    JsonObject *obj = json_node_get_object(node);

    if (!ci_config_load_settings(node))
        return FALSE;
    if (json_object_has_member(obj, "elements") &&
            !ci_config_load_element_array(json_object_get_member(obj, "elements")))
        return FALSE;
    if (json_object_has_member(obj, "list") &&
            !ci_config_load_list(json_object_get_member(obj, "list")))
        return FALSE;
    return TRUE;
}

gboolean ci_config_load_settings(JsonNode *node)
{
    if (!JSON_NODE_HOLDS_OBJECT(node))
        return FALSE;
    JsonObject *obj = json_node_get_object(node);

    GList *group;

    for (group = ci_config_groups; group != NULL; group = g_list_next(group)) {
        if (json_object_has_member(obj, ((struct CIConfigGroup*)group->data)->groupname))
            if (!ci_config_load_group(json_object_get_member(obj, ((struct CIConfigGroup*)group->data)->groupname),
                        (struct CIConfigGroup*)group->data))
                return FALSE;
    }

    return TRUE;
}

gboolean ci_config_load_group(JsonNode *node, struct CIConfigGroup *group)
{
    if (!JSON_NODE_HOLDS_OBJECT(node))
        return FALSE;
    JsonObject *obj = json_node_get_object(node);

    GList *member;
    struct CIConfigVariable *setting;

    for (member = group->variables; member != NULL; member = g_list_next(member)) {
        setting = (struct CIConfigVariable*)member->data;

        if (setting != NULL && json_object_has_member(obj, setting->key)) {
            switch (setting->type) {
                case CIConfigTypeInt:
                    ci_config_variable_set(setting,
                            GINT_TO_POINTER((gint)json_object_get_int_member(obj, setting->key)));
                    break;
                case CIConfigTypeUint:
                    ci_config_variable_set(setting,
                            GUINT_TO_POINTER((guint)json_object_get_int_member(obj, setting->key)));
                    break;
                case CIConfigTypeString:
                    ci_config_variable_set(setting,
                            (gpointer)json_object_get_string_member(obj, setting->key));
                    break;
                case CIConfigTypeColor:
                    {
                        GdkRGBA col;
                        ci_string_to_color(&col, json_object_get_string_member(obj, setting->key));
                        ci_config_variable_set(setting, (gpointer)&col);
                    }
                    break;
                default:
                    break;
            }
        }
    }

    return TRUE;
}

gboolean ci_config_load_element_array(JsonNode *node)
{
    if (!JSON_NODE_HOLDS_ARRAY(node))
        return FALSE;
    JsonArray *arr = json_node_get_array(node);
    GList *elements = json_array_get_elements(arr);
    GList *tmp;

    gboolean result = TRUE;
    for (tmp = elements; tmp != NULL; tmp = g_list_next(tmp)) {
        if (!ci_config_load_element((JsonNode*)tmp->data)) {
            result = FALSE;
            break;
        }
    }

    g_list_free(elements);

    return result;
}

gboolean ci_config_load_element(JsonNode *node)
{
    if (!JSON_NODE_HOLDS_OBJECT(node))
        return FALSE;
    JsonObject *obj = json_node_get_object(node);
    CIDisplayElement *element = ci_display_element_new();

    gdouble x, y;

    x = 0.0; y = 0.0;
    if (json_object_has_member(obj, "x"))
        x = (gint)json_object_get_int_member(obj, "x");
    if (json_object_has_member(obj, "y"))
        y = (gint)json_object_get_int_member(obj, "y");
    ci_display_element_set_pos(element, x, y);

    if (json_object_has_member(obj, "font"))
        ci_display_element_set_font(element, json_object_get_string_member(obj, "font"));

    if (json_object_has_member(obj, "format"))
        ci_display_element_set_format(element, json_object_get_string_member(obj, "format"));

    if (json_object_has_member(obj, "action"))
        ci_display_element_set_action(element, json_object_get_string_member(obj, "action"));

    GdkRGBA col;
    if (json_object_has_member(obj, "color")) {
        ci_string_to_color(&col, json_object_get_string_member(obj, "color"));
        ci_display_element_set_color(element, &col);
    }

    return TRUE;
}

gboolean ci_config_load_list(JsonNode *node)
{
    if (!JSON_NODE_HOLDS_OBJECT(node))
        return FALSE;
    JsonObject *obj = json_node_get_object(node);

    if (json_object_has_member(obj, "linecount"))
        ci_call_list_set_line_count(json_object_get_int_member(obj, "linecount"));
    
    if (json_object_has_member(obj, "font"))
        ci_call_list_set_font(json_object_get_string_member(obj, "font"));

    GdkRGBA col;
    if (json_object_has_member(obj, "color")) {
        ci_string_to_color(&col, json_object_get_string_member(obj, "color"));
        ci_call_list_set_color(&col);
    }

    if (json_object_has_member(obj, "columns"))
        return ci_config_load_list_column_array(json_object_get_member(obj, "columns"));

    return TRUE;
}

gboolean ci_config_load_list_column_array(JsonNode *node)
{
    if (!JSON_NODE_HOLDS_ARRAY(node))
        return FALSE;
    JsonArray *arr = json_node_get_array(node);
    GList *columns = json_array_get_elements(arr);
    GList *tmp;

    gboolean result = TRUE;
    for (tmp = columns; tmp != NULL; tmp = g_list_next(tmp)) {
        if (!ci_config_load_list_column((JsonNode*)tmp->data)) {
            result = FALSE;
            break;
        }
    }

    g_list_free(columns);

    return result;
}

gboolean ci_config_load_list_column(JsonNode *node)
{
    if (!JSON_NODE_HOLDS_OBJECT(node))
        return FALSE;
    JsonObject *obj = json_node_get_object(node);
    CICallListColumn *column = ci_call_list_append_column();

    if (json_object_has_member(obj, "format"))
        ci_call_list_set_column_format(column, json_object_get_string_member(obj, "format"));

    if (json_object_has_member(obj, "width"))
        ci_call_list_set_column_width(column, json_object_get_double_member(obj, "width"));

    return TRUE;
}

JsonNode *ci_config_save_root(void)
{
    JsonBuilder *builder = json_builder_new();
    JsonNode *root = NULL;

    json_builder_begin_object(builder);

    ci_config_save_settings(builder);
    
    json_builder_set_member_name(builder, "elements");
    ci_config_save_element_array(builder);

    json_builder_set_member_name(builder, "list");
    ci_config_save_list(builder);

    json_builder_end_object(builder);

    root = json_builder_get_root(builder);

    g_object_unref(builder);

    return root;
}

void ci_config_save_settings(JsonBuilder *builder)
{
    GList *group;

    for (group = ci_config_groups; group != NULL; group = g_list_next(group)) {
        ci_config_save_group(builder, (struct CIConfigGroup*)group->data);
    }
}

void ci_config_save_group(JsonBuilder *builder, struct CIConfigGroup *group)
{
    if (group == NULL)
        return;
    json_builder_set_member_name(builder, group->groupname);
    json_builder_begin_object(builder);

    gint ival;
    guint uival;
    gchar *strval;
    GdkRGBA col;

    GList *member;
    struct CIConfigVariable *setting;

    for (member = group->variables; member != NULL; member = g_list_next(member)) {
        setting = (struct CIConfigVariable*)member->data;

        if (setting == NULL)
            continue;

        json_builder_set_member_name(builder, setting->key);
        switch (setting->type) {
            case CIConfigTypeInt:
                ci_config_variable_get(setting, (gpointer)&ival, FALSE);
                json_builder_add_int_value(builder, ival);
                break;
            case CIConfigTypeUint:
                ci_config_variable_get(setting, (gpointer)&uival, FALSE);
                json_builder_add_int_value(builder, uival);
                break;
            case CIConfigTypeString:
                strval = NULL;
                ci_config_variable_get(setting, (gpointer)&strval, FALSE);
                json_builder_add_string_value(builder, strval);
                g_free(strval);
                break;
            case CIConfigTypeColor:
                ci_config_variable_get(setting, (gpointer)&col, FALSE);
                strval = ci_color_to_string(&col);
                json_builder_add_string_value(builder, strval);
                g_free(strval);
                break;
            default:
                json_builder_add_null_value(builder);
        }
    }

    json_builder_end_object(builder);
}

void ci_config_save_element_array(JsonBuilder *builder)
{
    json_builder_begin_array(builder);

    GList *elements = ci_display_element_get_elements();
    GList *tmp;

    for (tmp = g_list_last(elements); tmp != NULL; tmp = g_list_previous(tmp)) {
        ci_config_save_element(builder, (CIDisplayElement*)tmp->data);
    }

    json_builder_end_array(builder);
}

void ci_config_save_element(JsonBuilder *builder, CIDisplayElement *element)
{
    gdouble x, y;
    GdkRGBA col;
    json_builder_begin_object(builder);

    ci_display_element_get_pos(element, &x, &y);

    json_builder_set_member_name(builder, "x");
    json_builder_add_double_value(builder, x);

    json_builder_set_member_name(builder, "y");
    json_builder_add_double_value(builder, y);

    /* font, color, format, action */
    json_builder_set_member_name(builder, "font");
    json_builder_add_string_value(builder, ci_display_element_get_font(element));

    json_builder_set_member_name(builder, "format");
    json_builder_add_string_value(builder, ci_display_element_get_format(element));

    json_builder_set_member_name(builder, "action");
    json_builder_add_string_value(builder, ci_display_element_get_action(element));

    ci_display_element_get_color(element, &col);
    gchar *colstr = ci_color_to_string(&col);
    json_builder_set_member_name(builder, "color");
    json_builder_add_string_value(builder, colstr);
    g_free(colstr);
    
    json_builder_end_object(builder);
}

void ci_config_save_list(JsonBuilder *builder)
{
    json_builder_begin_object(builder);

    /* linecount, font, color, columns */
    json_builder_set_member_name(builder, "linecount");
    json_builder_add_int_value(builder, ci_call_list_get_line_count());

    json_builder_set_member_name(builder, "font");
    json_builder_add_string_value(builder, ci_call_list_get_font());

    GdkRGBA col;
    ci_call_list_get_color(&col);
    gchar *colstr = ci_color_to_string(&col);
    json_builder_set_member_name(builder, "color");
    json_builder_add_string_value(builder, colstr);
    g_free(colstr);

    json_builder_set_member_name(builder, "columns");
    ci_config_save_list_column_array(builder);

    json_builder_end_object(builder);
}

void ci_config_save_list_column_array(JsonBuilder *builder)
{
    json_builder_begin_array(builder);

    GList *columns;

    for (columns = ci_call_list_get_columns(); columns != NULL; columns = g_list_next(columns)) {
        ci_config_save_list_column(builder, (CICallListColumn*)columns->data);
    }

    json_builder_end_array(builder);
}

void ci_config_save_list_column(JsonBuilder *builder, CICallListColumn *column)
{
    json_builder_begin_object(builder);

    json_builder_set_member_name(builder, "format");
    json_builder_add_string_value(builder, ci_call_list_get_column_format(column));

    json_builder_set_member_name(builder, "width");
    json_builder_add_double_value(builder, ci_call_list_get_column_width(column));

    json_builder_end_object(builder);
}
