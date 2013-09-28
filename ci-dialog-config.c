#include "ci-dialog-config.h"
#include "ci-config.h"
#include "ci-logging.h"
#include "ci-window.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "gtk2-compat.h"
#include "ci-dialogs.h"

struct CIDialogConfigSetting {
    gchar *key;
    CIConfigSetting *setting;
    gpointer new_value;
    guint32 flags;
};

enum CIDialogConfigFlags {
    CIDialogConfigFlagChanged = 1 << 0,
    CIDialogConfigFlagSetDefault = 1 << 1
};

enum {
    ROW_KEY,
    ROW_VALUE,
    ROW_DEFAULT_VALUE,
    ROW_SETTING,
    N_ROWS
};

GtkWidget *dialog_config = NULL;

void ci_dialog_config_setting_free(gpointer data);
GList *ci_dialog_config_get_config(void);

void ci_dialog_config_set_row(GtkListStore *store, GtkTreeIter *iter, struct CIDialogConfigSetting *setting);
GtkListStore *ci_dialog_config_create_store(GList *settings);
GtkWidget *ci_dialog_config_create_list(GList *settings);
GtkWidget *ci_dialog_config_init(GList *settings);

void ci_dialog_config_row_activated_cb(GtkTreeView *view,
                                       GtkTreePath *path,
                                       GtkTreeViewColumn *column,
                                       gpointer userdata);

gboolean ci_dialog_config_edit_setting(struct CIDialogConfigSetting *setting);
gboolean ci_dialog_config_edit_setting_boolean(CIConfigType type, struct CIDialogConfigSetting *setting);
gboolean ci_dialog_config_edit_setting_int_or_string(CIConfigType type, struct CIDialogConfigSetting *setting);
gboolean ci_dialog_config_edit_setting_color(CIConfigType type, struct CIDialogConfigSetting *setting);

void ci_dialog_config_setting_free(gpointer data)
{
    if (data == NULL)
        return;
    g_free(((struct CIDialogConfigSetting*)data)->key);
    CIConfigType type;
    if (((struct CIDialogConfigSetting*)data)->new_value != NULL) {
        type = ci_config_setting_get_type(((struct CIDialogConfigSetting*)data)->setting);
        if (type == CIConfigTypeString || type == CIConfigTypeColor)
            g_free(((struct CIDialogConfigSetting*)data)->new_value);
    }
    g_free(data);
}

GList *ci_dialog_config_get_config(void)
{
    GList *settings = NULL;
    GList *keys = ci_config_enum_settings();
    GList *key;

    keys = g_list_reverse(keys);
    struct CIDialogConfigSetting *var;

    for (key = keys; key != NULL; key = g_list_next(key)) {
        var = g_malloc0(sizeof(struct CIDialogConfigSetting));
        var->key = (gchar*)key->data;
        var->setting = ci_config_get_setting(var->key);

        settings = g_list_prepend(settings, var);
    }

    g_list_free(keys);

    return settings;
}

void ci_dialog_config_row_activated_cb(GtkTreeView *view,
                                       GtkTreePath *path,
                                       GtkTreeViewColumn *column,
                                       gpointer userdata)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(view);

    if (model == NULL ||
           !gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path))
        return;

    struct CIDialogConfigSetting *setting;
    gtk_tree_model_get(model, &iter, ROW_SETTING, &setting, -1);

    if (setting == NULL)
        return;

    DLOG("row activated: \"%s\"\n", setting->key);
    if (ci_dialog_config_edit_setting(setting)) {
        ci_dialog_config_set_row(GTK_LIST_STORE(model), &iter, setting);
    }
}

void ci_dialog_config_set_row(GtkListStore *store, GtkTreeIter *iter, struct CIDialogConfigSetting *setting)
{
    gchar *value;
    CIConfigType type = ci_config_setting_get_type(setting->setting);

    if (setting->flags & CIDialogConfigFlagChanged)
        value = ci_config_setting_to_string(type, setting->new_value);
    else {
        value = ci_config_setting_get_value_as_string(setting->setting);
    }

    gchar *default_value = ci_config_setting_get_default_value_as_string(setting->setting);
    
    gtk_list_store_set(store, iter,
            ROW_KEY, setting->key,
            ROW_VALUE, value,
            ROW_DEFAULT_VALUE, default_value,
            ROW_SETTING, setting,
            -1);
}

GtkListStore *ci_dialog_config_create_store(GList *settings)
{
    GtkListStore *store = gtk_list_store_new(N_ROWS,
            G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

    GtkTreeIter iter;

    GList *tmp;
    for (tmp = settings; tmp != NULL; tmp = g_list_next(tmp)) {
        gtk_list_store_append(store, &iter);
        ci_dialog_config_set_row(store, &iter, (struct CIDialogConfigSetting*)tmp->data);
    }

    return store;
}

GtkWidget *ci_dialog_config_create_list(GList *settings)
{
    GtkListStore *store = ci_dialog_config_create_store(settings);
    GtkWidget *list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);

    GtkCellRenderer *cr = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col;

    col = gtk_tree_view_column_new_with_attributes(_("Setting"), cr,
            "text", ROW_KEY, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), col);

    col = gtk_tree_view_column_new_with_attributes(_("Value"), cr,
            "text", ROW_VALUE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), col);

    col = gtk_tree_view_column_new_with_attributes(_("Default value"), cr,
            "text", ROW_DEFAULT_VALUE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), col);

    return list;
}

GtkWidget *ci_dialog_config_init(GList *settings)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Configuration"),
            GTK_WINDOW(ci_window_get_window()),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            NULL);

    GtkWidget *list = ci_dialog_config_create_list(settings);
    g_signal_connect(G_OBJECT(list), "row-activated",
                     G_CALLBACK(ci_dialog_config_row_activated_cb), NULL);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll),
            GTK_SHADOW_ETCHED_IN);

    gtk_widget_set_size_request(scroll, 500, 300);

    gtk_container_add(GTK_CONTAINER(scroll), list);
    gtk_widget_show_all(scroll);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(content_area), scroll);

    return dialog;
}

void ci_dialog_config_show(CIConfigChangedCallback changed_cb)
{
    GList *settings = ci_dialog_config_get_config();
    dialog_config = ci_dialog_config_init(settings);
    GList *var;
    struct CIDialogConfigSetting *set;

    GtkResponseType result = gtk_dialog_run(GTK_DIALOG(dialog_config));

    if (result == GTK_RESPONSE_APPLY) {
        for (var = settings; var != NULL; var = g_list_next(var)) {
            set = (struct CIDialogConfigSetting*)var->data;
            if (set->flags & CIDialogConfigFlagChanged)
                ci_config_setting_set_value(set->setting, set->new_value);
        }
        ci_config_save();
    }

    gtk_widget_destroy(dialog_config);
    dialog_config = NULL;

    g_list_free_full(settings, ci_dialog_config_setting_free);

    if (result == GTK_RESPONSE_APPLY && changed_cb != NULL)
        changed_cb();
}

gboolean ci_dialog_config_edit_setting(struct CIDialogConfigSetting *setting)
{
    if (setting == NULL)
        return FALSE;
    CIConfigType type = ci_config_setting_get_type(setting->setting);

    switch (type) {
        case CIConfigTypeInt:
        case CIConfigTypeUint:
        case CIConfigTypeString:
            return ci_dialog_config_edit_setting_int_or_string(type, setting);
        case CIConfigTypeColor:
            return ci_dialog_config_edit_setting_color(type, setting);
        case CIConfigTypeBoolean:
            return ci_dialog_config_edit_setting_boolean(type, setting);
        default:
            return FALSE;
    }
}

GtkWidget *ci_dialog_config_edit_setting_dialog(gchar *key)
{
    gchar *title = g_strdup_printf(_("Edit setting: %s"), key);
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title,
            GTK_WINDOW(dialog_config),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            NULL);
    g_free(title);

    GtkWidget *default_button = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_APPLY);

    if (default_button != NULL) {
        gtk_widget_set_can_default(default_button, TRUE);
        gtk_widget_grab_default(default_button);
    }

    return dialog;
}

GtkResponseType ci_dialog_config_edit_setting_dialog_pack_and_run(GtkWidget *dialog, GtkWidget *content)
{
    GtkWidget *content_area = NULL;
    if (content != NULL) {
        gtk_widget_show_all(content);

        content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_container_add(GTK_CONTAINER(content_area), content);
    }

    return gtk_dialog_run(GTK_DIALOG(dialog));
}

gboolean ci_dialog_config_edit_setting_boolean(CIConfigType type, struct CIDialogConfigSetting *setting)
{
    GtkWidget *dialog = ci_dialog_config_edit_setting_dialog(setting->key);

    gboolean current_value;
    if (setting->flags & CIDialogConfigFlagChanged)
        current_value = (gboolean)GPOINTER_TO_INT(setting->new_value);
    else
        ci_config_setting_get_value(setting->setting, &current_value);

    GtkWidget *combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "true");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "false");

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), current_value ? 0 : 1);

    GtkResponseType result = ci_dialog_config_edit_setting_dialog_pack_and_run(dialog, combo);

    if (result == GTK_RESPONSE_APPLY) {
        setting->new_value = GINT_TO_POINTER((gint)(gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 0 ?
                    TRUE : FALSE));
        setting->flags |= CIDialogConfigFlagChanged;
    }

    gtk_widget_destroy(dialog);

    return (result == GTK_RESPONSE_APPLY);
}

gboolean ci_dialog_config_edit_setting_int_or_string(CIConfigType type, struct CIDialogConfigSetting *setting)
{
    GtkWidget *dialog = ci_dialog_config_edit_setting_dialog(setting->key);

    gchar *current_value = NULL;
    if (setting->flags & CIDialogConfigFlagChanged)
        current_value = ci_config_setting_to_string(type, setting->new_value);
    else
        current_value = ci_config_setting_get_value_as_string(setting->setting);

    GtkEntryBuffer *entry_buffer = gtk_entry_buffer_new(current_value, -1);
    GtkWidget *entry = gtk_entry_new_with_buffer(entry_buffer);
    g_free(current_value);

    g_object_set(G_OBJECT(entry), "activates-default", TRUE, NULL);

    GtkResponseType result = ci_dialog_config_edit_setting_dialog_pack_and_run(dialog, entry);

    if (result == GTK_RESPONSE_APPLY) {
        current_value = (gchar *)gtk_entry_buffer_get_text(entry_buffer);
        ci_config_setting_from_string(type, &setting->new_value, current_value);
        setting->flags |= CIDialogConfigFlagChanged;
    }

    g_object_unref(G_OBJECT(entry_buffer));

    gtk_widget_destroy(dialog);

    return (result == GTK_RESPONSE_APPLY);
}

gboolean ci_dialog_config_edit_setting_color(CIConfigType type, struct CIDialogConfigSetting *setting)
{
    gchar *title = g_strdup_printf(_("Edit setting: %s"), setting->key);
    GdkRGBA col;

    if (setting->flags & CIDialogConfigFlagChanged)
        memcpy(&col, setting->new_value, sizeof(GdkRGBA));
    else
        ci_config_setting_get_value(setting->setting, &col);

    gboolean result = ci_dialog_choose_color_dialog(title, &col);
    if (result == TRUE) {
        if (setting->new_value == NULL)
            setting->new_value = g_malloc0(sizeof(GdkRGBA));
        memcpy(setting->new_value, &col, sizeof(GdkRGBA));
        setting->flags |= CIDialogConfigFlagChanged;
    }

    return result;
}
