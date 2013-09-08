#include "ci-dialogs.h"
#include "ci-window.h"

gboolean ci_dialogs_add_caller(CICallerInfo *caller)
{
    if (caller == NULL)
        return FALSE;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Add Caller",
            GTK_WINDOW(ci_window_get_window()),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_STOCK_SAVE, GTK_RESPONSE_APPLY,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            NULL);

    /* grid */
    GtkWidget *grid = gtk_grid_new();
    GtkWidget *label, *content_area;
    GtkWidget *entries[2];
    GtkEntryBuffer *buffers[2];

    buffers[0] = gtk_entry_buffer_new(caller->number, -1);
    buffers[1] = gtk_entry_buffer_new(caller->name, -1);

    entries[0] = gtk_entry_new_with_buffer(buffers[0]);
    entries[1] = gtk_entry_new_with_buffer(buffers[1]);

    /* padding, align */
    label = gtk_label_new("Number:");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entries[0], 1, 0, 1, 1);

    label = gtk_label_new("Name:");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entries[1], 1, 1, 1, 1);

    gtk_widget_show_all(grid);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    gtk_container_add(GTK_CONTAINER(content_area), grid);

/*    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_APPLY);
    gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_APPLY, TRUE);
*/
    GtkResponseType result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_APPLY) {
        g_free(caller->number);
        g_free(caller->name);
        caller->number = g_strdup(gtk_entry_buffer_get_text(buffers[0]));
        caller->name = g_strdup(gtk_entry_buffer_get_text(buffers[1]));
    }

    g_object_unref(G_OBJECT(buffers[0]));
    g_object_unref(G_OBJECT(buffers[1]));

    gtk_widget_destroy(dialog);

    return (result == GTK_RESPONSE_APPLY);
}