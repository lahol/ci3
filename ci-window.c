#include "ci-window.h"
#include "ci-display-elements.h"

GtkWidget *window = NULL;
GtkWidget *darea;

gint win_x = 0;
gint win_y = 0;
gint win_w = 0;
gint win_h = 0;

gboolean ci_window_event_draw(GtkWidget *widget,
        cairo_t *cr, gpointer userdata)
{
    guint width, height;

    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);

    cairo_rectangle(cr, 0, 0, width, height);
    cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);

    cairo_fill(cr);

    ci_display_element_render_all(cr);

    return FALSE;
}

gboolean ci_window_event_delete_event(GtkWidget *widget, GdkEvent *event, gpointer userdata)
{
    ci_window_hide();
    return TRUE;
}

gboolean ci_window_init(gint x, gint y, gint w, gint h)
{
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (window == NULL)
        return FALSE;

    gtk_window_set_focus_on_map(GTK_WINDOW(window), FALSE);

    win_x = x;
    win_y = y;
    win_w = w;
    win_h = h;

    darea = gtk_drawing_area_new();

    gtk_widget_add_events(GTK_WIDGET(darea),
            GDK_BUTTON_PRESS_MASK |
            GDK_BUTTON_RELEASE_MASK |
            GDK_KEY_PRESS_MASK |
            GDK_KEY_RELEASE_MASK |
            GDK_SCROLL_MASK);

    gtk_container_add(GTK_CONTAINER(window), darea);
    gtk_widget_show(darea);

    g_signal_connect(G_OBJECT(darea), "draw", G_CALLBACK(ci_window_event_draw), NULL);
    g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(ci_window_event_delete_event), NULL);
     /* g_signal_connect(G_OBJECT(darea), "button-press-event", "scroll-event", "destroy" */
    gtk_window_set_role(GTK_WINDOW(window), "MainWindow");

    return TRUE;
}

void ci_window_show(gboolean mark_as_urgent, gboolean focus)
{
    if (window) {
        gtk_window_present(GTK_WINDOW(window));
        gtk_window_move(GTK_WINDOW(window), win_x, win_y);
        if (mark_as_urgent)
            gtk_window_set_urgency_hint(GTK_WINDOW(window), TRUE);
    }
}

void ci_window_update(void)
{
    if (window && gtk_widget_get_visible(window))
        gtk_widget_queue_draw(window);
}

void ci_window_hide(void)
{
    if (window) {
        gtk_window_get_position(GTK_WINDOW(window), &win_x, &win_y);
        gtk_widget_hide(window);
    }
}

void ci_window_destroy(void)
{
}
