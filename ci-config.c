#include "ci-config.h"
#include <json-glib/json-glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include "ci-display-elements.h"
#include "ci-call-list.h"
#include <memory.h>
#include "gtk2-compat.h"

struct CIConfig {
    gchar *host;
    guint port;
    GdkRGBA background;
    gint win_x;
    gint win_y;
    gint win_w;
    gint win_h;
} ci_config;

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
    g_free(ci_config.host);
    ci_config.host = g_strdup("localhost");
    ci_config.port = 63690;
    ci_config.win_x = 0;
    ci_config.win_y = 0;
    ci_config.win_w = 200;
    ci_config.win_h = 100;
    ci_string_to_color(&ci_config.background, "#ffffff");
}

gboolean ci_config_load_root(JsonNode *node);
gboolean ci_config_load_client(JsonNode *node);
gboolean ci_config_load_window(JsonNode *node);
gboolean ci_config_load_element_array(JsonNode *node);
gboolean ci_config_load_element(JsonNode *node);
gboolean ci_config_load_list(JsonNode *node);
gboolean ci_config_load_list_column_array(JsonNode *node);
gboolean ci_config_load_list_column(JsonNode *node);

JsonNode *ci_config_save_root(void);
void ci_config_save_client(JsonBuilder *builder);
void ci_config_save_window(JsonBuilder *builder);
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
    if (!json_parser_load_from_file(parser, filename, &err)) {
        g_printf("load from file failed: %s\n", err->message);
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
        g_printf("save to file failed: %s\n", err->message);
        g_error_free(err);
        result = FALSE;
    }

    json_node_free(root);
    g_object_unref(gen);
    g_free(filename);

    return result;
}

gboolean ci_config_get(const gchar *key, gpointer value)
{
    if (g_strcmp0(key, "client:host") == 0) {
        *((gchar**)value) = g_strdup(ci_config.host);
    }
    else if (g_strcmp0(key, "client:port") == 0) {
        *((guint*)value) = ci_config.port;
    }
    else if (g_strcmp0(key, "window:background") == 0) {
        memcpy(value, &ci_config.background, sizeof(GdkRGBA));
    }
    else if (g_strcmp0(key, "window:x") == 0) {
        *((gint*)value) = ci_config.win_x;
    }
    else if (g_strcmp0(key, "window:y") == 0) {
        *((gint*)value) = ci_config.win_y;
    }
    else if (g_strcmp0(key, "window:width") == 0) {
        *((gint*)value) = ci_config.win_w;
    }
    else if (g_strcmp0(key, "window:height") == 0) {
        *((gint*)value) = ci_config.win_h;
    }
    else {
        return FALSE;
    }
    return TRUE;
}

gboolean ci_config_set(const gchar *key, gpointer value)
{
    if (g_strcmp0(key, "client:host") == 0) {
        g_free(ci_config.host);
        ci_config.host = g_strdup((gchar*)value);
    }
    else if (g_strcmp0(key, "client:port") == 0) {
        ci_config.port = GPOINTER_TO_UINT(value);
    }
    else if (g_strcmp0(key, "window:background") == 0) {
        memcpy(&ci_config.background, value, sizeof(GdkRGBA));
    }
    else if (g_strcmp0(key, "window:x") == 0) {
        ci_config.win_x = GPOINTER_TO_INT(value);
    }
    else if (g_strcmp0(key, "window:y") == 0) {
        ci_config.win_y = GPOINTER_TO_INT(value);
    }
    else if (g_strcmp0(key, "window:width") == 0) {
        ci_config.win_w = GPOINTER_TO_INT(value);
    }
    else if (g_strcmp0(key, "window:height") == 0) {
        ci_config.win_h = GPOINTER_TO_INT(value);
    }
    else {
        return FALSE;
    }
    return TRUE;
}

gboolean ci_config_load_root(JsonNode *node)
{
    if (!JSON_NODE_HOLDS_OBJECT(node))
        return FALSE;
    JsonObject *obj = json_node_get_object(node);
    if (json_object_has_member(obj, "client") &&
            !ci_config_load_client(json_object_get_member(obj, "client")))
        return FALSE;
    if (json_object_has_member(obj, "window") &&
            !ci_config_load_window(json_object_get_member(obj, "window")))
        return FALSE;
    if (json_object_has_member(obj, "elements") &&
            !ci_config_load_element_array(json_object_get_member(obj, "elements")))
        return FALSE;
    if (json_object_has_member(obj, "list") &&
            !ci_config_load_list(json_object_get_member(obj, "list")))
        return FALSE;
    return TRUE;
}

gboolean ci_config_load_client(JsonNode *node)
{
    if (!JSON_NODE_HOLDS_OBJECT(node))
        return FALSE;
    JsonObject *obj = json_node_get_object(node);

    if (json_object_has_member(obj, "host"))
        ci_config_set("client:host", (gpointer)json_object_get_string_member(obj, "host"));
    if (json_object_has_member(obj, "port"))
        ci_config_set("client:port", GUINT_TO_POINTER((guint)json_object_get_int_member(obj, "port")));

    return TRUE;
}

gboolean ci_config_load_window(JsonNode *node)
{
    if (!JSON_NODE_HOLDS_OBJECT(node))
        return FALSE;
    JsonObject *obj = json_node_get_object(node);
    GdkRGBA col;

    if (json_object_has_member(obj, "background")) {
        ci_string_to_color(&col, json_object_get_string_member(obj, "background"));
        ci_config_set("window:background", (gpointer)&col);
    }
    if (json_object_has_member(obj, "x"))
        ci_config_set("window:x", GINT_TO_POINTER((gint)json_object_get_int_member(obj, "x")));
    if (json_object_has_member(obj, "y"))
        ci_config_set("window:y", GINT_TO_POINTER((gint)json_object_get_int_member(obj, "y")));
    if (json_object_has_member(obj, "width"))
        ci_config_set("window:width", GINT_TO_POINTER((gint)json_object_get_int_member(obj, "width")));
    if (json_object_has_member(obj, "height"))
        ci_config_set("window:height", GINT_TO_POINTER((gint)json_object_get_int_member(obj, "height")));
    
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

    json_builder_set_member_name(builder, "client");
    ci_config_save_client(builder);

    json_builder_set_member_name(builder, "window");
    ci_config_save_window(builder);

    json_builder_set_member_name(builder, "elements");
    ci_config_save_element_array(builder);

    json_builder_set_member_name(builder, "list");
    ci_config_save_list(builder);

    json_builder_end_object(builder);

    root = json_builder_get_root(builder);

    g_object_unref(builder);

    return root;
}

void ci_config_save_client(JsonBuilder *builder)
{
    json_builder_begin_object(builder);

    json_builder_set_member_name(builder, "host");
    json_builder_add_string_value(builder, ci_config.host);

    json_builder_set_member_name(builder, "port");
    json_builder_add_int_value(builder, ci_config.port);

    json_builder_end_object(builder);
}

void ci_config_save_window(JsonBuilder *builder)
{
    json_builder_begin_object(builder);

    json_builder_set_member_name(builder, "x");
    json_builder_add_int_value(builder, ci_config.win_x);

    json_builder_set_member_name(builder, "y");
    json_builder_add_int_value(builder, ci_config.win_y);

    json_builder_set_member_name(builder, "width");
    json_builder_add_int_value(builder, ci_config.win_w);

    json_builder_set_member_name(builder, "height");
    json_builder_add_int_value(builder, ci_config.win_h);

    gchar *strcol = ci_color_to_string(&ci_config.background);
    json_builder_set_member_name(builder, "background");
    json_builder_add_string_value(builder, strcol);
    g_free(strcol);

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
