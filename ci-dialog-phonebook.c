#include "ci-dialog-phonebook.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "ci-window.h"
#include "ci-client.h"
#include "ci-logging.h"
#include "ci-config.h"
#include "ci-dialogs.h"
#include <cinet.h>

struct CIDialogPhonebook {
    GtkWidget *dialog;
    GtkWidget *treeview;
    GtkListStore *store;
    GtkTreeModel *filter;
    GtkWidget *filter_entry;
    GList *callers;
    void (*refresh_cb)(void);
};

enum {
    ROW_NAME,
    ROW_NUMBER,
    ROW_DATA,
    N_ROWS
};

void ci_dialog_phonebook_update_list(struct CIDialogPhonebook *data);

void ci_dialog_phonebook_clear_caller_list(struct CIDialogPhonebook *data)
{
    if (data == NULL)
        return;
    g_list_free_full(data->callers, (GDestroyNotify)cinet_caller_info_free_full);
    data->callers = NULL;
}

void ci_dialog_phonebook_call_refresh(CINetMsg *msg, struct CIDialogPhonebook *data)
{
    ci_dialog_phonebook_update_list(data);
    if (data != NULL && data->refresh_cb)
        data->refresh_cb();
}

void ci_dialog_phonebook_caller_add(CICallerInfo *ci, struct CIDialogPhonebook *data)
{
    gint user = ci_config_get_int("client:user");

    if (ci_dialogs_add_caller(ci)) {
        client_query(CIClientQueryAddCaller,
                (CIQueryMsgCallback)ci_dialog_phonebook_call_refresh, data,
                "user", GINT_TO_POINTER(user),
                "number", ci->number,
                "name", ci->name,
                NULL, NULL);
    }
}

void ci_dialog_phonebook_caller_del(CICallerInfo *ci, struct CIDialogPhonebook *data)
{
    if (data == NULL || ci == NULL)
        return;

    gint user;

    GtkWidget *message = gtk_message_dialog_new(GTK_WINDOW(data->dialog),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("Really delete the entry %s (%s)?"),
            ci->name, ci->number);

    GtkResponseType result = gtk_dialog_run(GTK_DIALOG(message));
    if (result == GTK_RESPONSE_YES) {
        user = ci_config_get_int("client:user");
        client_query(CIClientQueryDelCaller,
                (CIQueryMsgCallback)ci_dialog_phonebook_call_refresh, data,
                "user", GINT_TO_POINTER(user),
                "number", ci->number,
                "name", ci->name,
                NULL, NULL);
    }

    gtk_widget_destroy(message);
}

void ci_dialog_phonebook_new_cb(GtkWidget *button, struct CIDialogPhonebook *data)
{
    CICallerInfo ci;
    cinet_caller_info_init(&ci);
    ci_dialog_phonebook_caller_add(&ci, data);
    cinet_caller_info_free(&ci);
}

CICallerInfo *ci_dialog_phonebook_get_selected(struct CIDialogPhonebook *data)
{
    if (data == NULL)
        return NULL;

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->treeview));
    GtkTreeModel *filter_model, *model;
    GtkTreeIter filter_iter, iter;

    if (!gtk_tree_selection_get_selected(selection, &filter_model, &filter_iter))
        return NULL;

    gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(filter_model),
            &iter, &filter_iter);
    model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter_model));

    CICallerInfo *info = NULL;
    gtk_tree_model_get(model, &iter, ROW_DATA, &info, -1);

    return info;
}

void ci_dialog_phonebook_edit_cb(GtkWidget *button, struct CIDialogPhonebook *data)
{
    CICallerInfo ci;
    cinet_caller_info_init(&ci);

    CICallerInfo *selection = ci_dialog_phonebook_get_selected(data);
    if (selection == NULL)
        return;
    cinet_caller_info_copy(&ci, selection);

    ci_dialog_phonebook_caller_add(&ci, data);
    cinet_caller_info_free(&ci);
}

void ci_dialog_phonebook_delete_cb(GtkWidget *button, struct CIDialogPhonebook *data)
{
    CICallerInfo ci;
    cinet_caller_info_init(&ci);

    CICallerInfo *selection = ci_dialog_phonebook_get_selected(data);
    if (selection == NULL)
        return;
    cinet_caller_info_copy(&ci, selection);

    ci_dialog_phonebook_caller_del(&ci, data);
    cinet_caller_info_free(&ci);
}

void ci_dialog_phonebook_response_cb(GtkDialog *dialog, gint response_id, struct CIDialogPhonebook *data)
{
    DLOG("phonebook response_id: %d\n", response_id);
    if (response_id == GTK_RESPONSE_CLOSE ||
            response_id == GTK_RESPONSE_DELETE_EVENT) {
        /* g_object_unref(data->filter_buffer); */
        ci_dialog_phonebook_clear_caller_list(data);
        g_object_unref(data->store);
        gtk_widget_destroy(GTK_WIDGET(dialog));

        g_free(data);
    }
}

void ci_dialog_phonebook_get_caller_list_cb(CINetMsgDbGetCallerList *msg, struct CIDialogPhonebook *data)
{
    DLOG("phonebook get caller list cb\n");
    gtk_list_store_clear(data->store);

    ci_dialog_phonebook_clear_caller_list(data);

    GList *caller;
    GtkTreeIter iter;
    CICallerInfo *info;
    for (caller = msg->callers; caller != NULL; caller = g_list_next(caller)) {
        info = cinet_caller_info_new();
        cinet_caller_info_copy(info, (CICallerInfo*)caller->data);
        data->callers = g_list_prepend(data->callers, info);
        gtk_list_store_append(data->store, &iter);
        gtk_list_store_set(data->store, &iter,
                ROW_NAME, info->name,
                ROW_NUMBER, info->number,
                ROW_DATA, info,
                -1);
    }

    data->callers = g_list_reverse(data->callers);
}

void ci_dialog_phonebook_update_list(struct CIDialogPhonebook *data)
{
    gint user = ci_config_get_int("client:user");
    client_query(CIClientQueryGetCallerList,
                 (CIQueryMsgCallback)ci_dialog_phonebook_get_caller_list_cb, data,
                 "user", GINT_TO_POINTER(user),
                 "filter", NULL,
                 NULL, NULL);
}

void ci_dialog_phonebook_row_activated_cb(GtkTreeView *view,
                                          GtkTreePath *path,
                                          GtkTreeViewColumn *column,
                                          struct CIDialogPhonebook *data)
{
    GtkTreeIter filter_iter, iter;
    GtkTreeModel *filter_model = NULL, *model = NULL;
    filter_model = gtk_tree_view_get_model(view);

    if (filter_model == NULL ||
            !gtk_tree_model_get_iter(filter_model, &filter_iter, path))
        return;

    gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(filter_model),
           &iter, &filter_iter);
    model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter_model));

    CICallerInfo *info = NULL;
    gtk_tree_model_get(model, &iter, ROW_DATA, &info, -1);

    if (info == NULL)
        return;

    CICallerInfo ci;
    cinet_caller_info_init(&ci);

    cinet_caller_info_copy(&ci, info);

    ci_dialog_phonebook_caller_add(&ci, data);
    cinet_caller_info_free(&ci);
}

gboolean ci_dialog_phonebook_filter_visible_func(GtkTreeModel *model,
                                                 GtkTreeIter *iter,
                                                 struct CIDialogPhonebook *data)
{
    if (data == NULL || data->filter_entry == NULL)
        return TRUE;

    gchar *filter = g_utf8_strdown(gtk_entry_get_text(GTK_ENTRY(data->filter_entry)), -1);
    if (filter == NULL || filter[0] == 0)
        return TRUE;

    CICallerInfo *info = NULL;
    gtk_tree_model_get(model, iter, ROW_DATA, &info, -1);
    if (info == NULL)
        return TRUE;

    gchar *number = g_utf8_strdown(info->number, -1);
    gchar *name   = g_utf8_strdown(info->name, -1);

    gboolean result = FALSE;
    if (g_strstr_len(number, -1, filter) != NULL ||
            g_strstr_len(name, -1, filter) != NULL)
        result = TRUE;

    g_free(number);
    g_free(name);

    return result;
}

void ci_dialog_phonebook_filter_changed_cb(GtkWidget *widget, struct CIDialogPhonebook *data)
{
    if (data != NULL && data->filter != NULL)
        gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(data->filter));
}

void ci_dialog_phonebook_show(void (*refresh_cb)(void))
{
    struct CIDialogPhonebook *phonebook = g_malloc0(sizeof(struct CIDialogPhonebook));

    phonebook->refresh_cb = refresh_cb;

    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), _("Phonebook"));
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(ci_window_get_window()));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
    phonebook->dialog = dialog;

    g_signal_connect(G_OBJECT(dialog), "response",
            G_CALLBACK(ci_dialog_phonebook_response_cb), phonebook);

    phonebook->filter_entry = gtk_entry_new();
    g_signal_connect(G_OBJECT(phonebook->filter_entry), "changed",
            G_CALLBACK(ci_dialog_phonebook_filter_changed_cb), phonebook);
    gtk_widget_show(phonebook->filter_entry);

    phonebook->store = gtk_list_store_new(N_ROWS, G_TYPE_STRING,
            G_TYPE_STRING, G_TYPE_POINTER);

    phonebook->filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(phonebook->store), NULL);
    gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(phonebook->filter),
            (GtkTreeModelFilterVisibleFunc)ci_dialog_phonebook_filter_visible_func,
            phonebook, NULL);

    phonebook->treeview= gtk_tree_view_new_with_model(GTK_TREE_MODEL(phonebook->filter));

    GtkCellRenderer *cr = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col;

    col = gtk_tree_view_column_new_with_attributes(_("Name"), cr,
            "text", ROW_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(phonebook->treeview), col);

    col = gtk_tree_view_column_new_with_attributes(_("Number"), cr,
            "text", ROW_NUMBER, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(phonebook->treeview), col);

    g_signal_connect(G_OBJECT(phonebook->treeview), "row-activated",
            G_CALLBACK(ci_dialog_phonebook_row_activated_cb), phonebook);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll),
            GTK_SHADOW_ETCHED_IN);

    gtk_widget_set_size_request(scroll, 500, 300);
    
    gtk_container_add(GTK_CONTAINER(scroll), phonebook->treeview);
    gtk_widget_show_all(scroll);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content_area), phonebook->filter_entry, FALSE, FALSE, 3);
    gtk_box_pack_end(GTK_BOX(content_area), scroll, TRUE, TRUE, 3);

    GtkWidget *button;
    GtkWidget *action_area = gtk_dialog_get_action_area(GTK_DIALOG(dialog));

    button = gtk_button_new_from_stock(GTK_STOCK_NEW);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(ci_dialog_phonebook_new_cb), phonebook);
    gtk_box_pack_end(GTK_BOX(action_area), button, TRUE, FALSE, 3);
    gtk_widget_show(button);

    button = gtk_button_new_from_stock(GTK_STOCK_EDIT);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(ci_dialog_phonebook_edit_cb), phonebook);
    gtk_box_pack_end(GTK_BOX(action_area), button, TRUE, FALSE, 3);
    gtk_widget_show(button);

    button = gtk_button_new_from_stock(GTK_STOCK_DELETE);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(ci_dialog_phonebook_delete_cb), phonebook);
    gtk_box_pack_end(GTK_BOX(action_area), button, TRUE, FALSE, 3);
    gtk_widget_show(button);

    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

    ci_dialog_phonebook_update_list(phonebook);

    gtk_widget_show_all(dialog);
}
