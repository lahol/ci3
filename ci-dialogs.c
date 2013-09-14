#include "ci-dialogs.h"
#include "ci-window.h"
#include "ci-config.h"
#include <glib/gi18n.h>

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
    GtkWidget *label, *content_area;
    GtkWidget *entries[2];
    GtkEntryBuffer *buffers[2];

    buffers[0] = gtk_entry_buffer_new(caller->number, -1);
    buffers[1] = gtk_entry_buffer_new(caller->name, -1);

    entries[0] = gtk_entry_new_with_buffer(buffers[0]);
    entries[1] = gtk_entry_new_with_buffer(buffers[1]);

    g_object_set(G_OBJECT(entries[0]), "activates-default", TRUE, NULL);
    g_object_set(G_OBJECT(entries[1]), "activates-default", TRUE, NULL);

    /* padding, align */
#if GTK_CHECK_VERSION(3, 0, 0)
    GtkWidget *grid = gtk_grid_new();
    label = gtk_label_new(_("Number:"));
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entries[0], 1, 0, 1, 1);

    label = gtk_label_new(_("Name:"));
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entries[1], 1, 1, 1, 1);
#else
    GtkWidget *grid = gtk_table_new(2, 2, FALSE);
    label = gtk_label_new(_("Number:"));
    gtk_table_attach_defaults(GTK_TABLE(grid), label, 0, 1, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(grid), entries[0], 1, 2, 0, 1);

    label = gtk_label_new(_("Name:"));
    gtk_table_attach_defaults(GTK_TABLE(grid), label, 0, 1, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(grid), entries[1], 1, 2, 1, 2);
#endif

    gtk_widget_show_all(grid);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    gtk_container_add(GTK_CONTAINER(content_area), grid);

    GtkWidget *default_button = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog),
                                                                   GTK_RESPONSE_APPLY);

    if (default_button != NULL) {
        gtk_widget_set_can_default(default_button, TRUE);
        gtk_widget_grab_default(default_button);
    }

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

void ci_dialogs_about(void)
{
    gchar *cfgfile, *comment = NULL;

    cfgfile = ci_config_get_string("config-file");
    comment = g_strconcat(_("Configuration file: "), cfgfile, NULL);
    gchar *authors[] = { "Holger Langenau", NULL };
#ifndef CIVERSION
#define CIVERSION "2.9"
#endif

    gtk_show_about_dialog(GTK_WINDOW(ci_window_get_window()),
            "program-name", "CallerInfo",
            "version", CIVERSION,
            "comments", comment,
            "authors", authors,
            NULL);

    g_free(comment);
    g_free(cfgfile);
}
