#include "ci-config.h"
#include <json-glib/json-glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include "ci-display-elements.h"
#include <memory.h>

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

gboolean ci_config_load(void)
{
    gboolean result = TRUE;
    gchar *filename = ci_config_get_config_file();
    JsonParser *parser = json_parser_new();
    JsonNode *root;
    GError *err = NULL;
    if (!json_parser_load_from_file(parser, filename, &err)) {
        g_print("load from file failed: %s\n", err->message);
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
    return TRUE;
}

gboolean ci_config_get(const gchar *key, gpointer value)
{
    g_printf("ci_config_get: %s\n", key);
    if (g_strcmp0(key, "client:host") == 0) {
        g_printf("host: %s\n", ci_config.host);
        *((gchar**)value) = g_strdup(ci_config.host);
    }
    else if (g_strcmp0(key, "client:port") == 0) {
        g_print("get port\n");
        *((guint*)value) = ci_config.port;
    }
    else if (g_strcmp0(key, "window:background") == 0) {
        g_print("get bknd\n");
        memcpy(value, &ci_config.background, sizeof(GdkRGBA));
    }
    else if (g_strcmp0(key, "window:x") == 0) {
        g_print("get x\n");
        *((gint*)value) = ci_config.win_x;
    }
    else if (g_strcmp0(key, "window:y") == 0) {
        g_print("get y\n");
        *((gint*)value) = ci_config.win_y;
    }
    else if (g_strcmp0(key, "window:width") == 0) {
        g_print("get w\n");
        *((gint*)value) = ci_config.win_w;
    }
    else if (g_strcmp0(key, "window:height") == 0) {
        g_print("get h\n");
        *((gint*)value) = ci_config.win_h;
    }
    else {
        g_print("nothing found\n");
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
