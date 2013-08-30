#include "ci-window.h"
#include "ci-menu.h"
#include "ci-display-elements.h"
#include <glib/gprintf.h>

GtkWidget *window = NULL;
GtkWidget *darea;

gint win_x = 0;
gint win_y = 0;
gint win_w = 0;
gint win_h = 0;

struct {
    gint start_x;
    gint start_y;
    GList *dragged_elements;
} drag_state;

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

gboolean ci_window_configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer userdata)
{
    win_x = event->x;
    win_y = event->y;
    win_x = event->width;
    win_y = event->height;

    return FALSE;
}

gboolean ci_window_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer userdata)
{
    CIDisplayElement *el = ci_display_element_get_from_pos(event->x, event->y);

    GtkWidget *ctx_menu = NULL;

    if (event->button == 3) {
        if (event->state & GDK_SHIFT_MASK)
            ctx_menu = ci_menu_context_menu((gpointer)el);
        else 
            ctx_menu = ci_menu_context_menu(NULL);


        gtk_widget_show_all(ctx_menu);
        gtk_menu_popup(GTK_MENU(ctx_menu), NULL, NULL, NULL, NULL, event->button, event->time);
    }
    if (event->button == 1 && event->state & GDK_SHIFT_MASK) {
        if (el) {
            g_print("begin drag\n");
            drag_state.dragged_elements = g_list_prepend(drag_state.dragged_elements, (gpointer)el);
            drag_state.start_x = event->x;
            drag_state.start_y = event->y;

            ci_display_element_drag_begin(el);
        }
    }

    return TRUE;
}

gboolean ci_window_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer userdata)
{
    GList *tmp;
    if ((event->state & GDK_BUTTON1_MASK) && (event->state & GDK_SHIFT_MASK)) {
        gdouble dx = (gdouble)(event->x - drag_state.start_x);
        gdouble dy = (gdouble)(event->y - drag_state.start_y);
        tmp = drag_state.dragged_elements;
        while (tmp) {
            ci_display_element_drag_update((CIDisplayElement*)tmp->data, dx, dy);
            tmp = g_list_next(tmp);
        }
        ci_window_update();
    }
    return TRUE;
}

gboolean ci_window_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer userdata)
{
    GList *tmp;
    if (event->button == 1 && (event->state & GDK_SHIFT_MASK)) {
        g_print("end drag\n");
        gdouble dx = (gdouble)(event->x - drag_state.start_x);
        gdouble dy = (gdouble)(event->y - drag_state.start_y);
        tmp = drag_state.dragged_elements;
        while (tmp) {
            ci_display_element_drag_update((CIDisplayElement*)tmp->data, dx, dy);
            ci_display_element_drag_finish((CIDisplayElement*)tmp->data);
            tmp = g_list_next(tmp);
        }
        g_list_free(drag_state.dragged_elements);
        drag_state.dragged_elements = NULL;
        ci_window_update();
    }
    return TRUE;
}

gboolean ci_window_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer userdata)
{
    switch (event->keyval) {
        case GDK_KEY_Escape:
            ci_window_hide();
            break;
        default:
            break;
    }
    return TRUE;
}

gboolean ci_window_key_release_event(GtkWidget *widget, GdkEventKey *event, gpointer userdata)
{
    switch (event->keyval) {
        case GDK_KEY_Shift_L:
        case GDK_KEY_Shift_R:
        case GDK_KEY_Caps_Lock:
            if ((event->state & GDK_BUTTON1_MASK) && (event->state & GDK_SHIFT_MASK)) {
                /* cancel drag */
                g_print("cancel drag\n");
                GList *tmp = drag_state.dragged_elements;
                while (tmp) {
                    ci_display_element_drag_update((CIDisplayElement*)tmp->data, 0.0, 0.0);
                    ci_display_element_drag_finish((CIDisplayElement*)tmp->data);
                    tmp = g_list_next(tmp);
                }
                g_list_free(drag_state.dragged_elements);
                drag_state.dragged_elements = NULL;

                ci_window_update();
            }
            break;
    }
    return TRUE;
}

gboolean ci_window_scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer userdata)
{
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
            GDK_POINTER_MOTION_MASK |
            GDK_KEY_PRESS_MASK |
            GDK_KEY_RELEASE_MASK |
            GDK_SCROLL_MASK);

    gtk_container_add(GTK_CONTAINER(window), darea);
    gtk_widget_show(darea);

    g_signal_connect(G_OBJECT(darea), "draw", G_CALLBACK(ci_window_event_draw), NULL);
    g_signal_connect(G_OBJECT(darea), "button-press-event", G_CALLBACK(ci_window_button_press_event), NULL);
    g_signal_connect(G_OBJECT(darea), "motion-notify-event", G_CALLBACK(ci_window_motion_notify_event), NULL);
    g_signal_connect(G_OBJECT(darea), "button-release-event", G_CALLBACK(ci_window_button_release_event), NULL);
    g_signal_connect(G_OBJECT(darea), "scroll-event", G_CALLBACK(ci_window_scroll_event), NULL);

    g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(ci_window_event_delete_event), NULL);
    g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(ci_window_configure_event), NULL);
    g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(ci_window_key_press_event), NULL);
    g_signal_connect(G_OBJECT(window), "key-release-event", G_CALLBACK(ci_window_key_release_event), NULL);
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
        gtk_widget_hide(window);
    }
}

void ci_window_destroy(void)
{
}

void ci_window_select_font_dialog(gpointer userdata)
{
    GtkWidget *dialog = gtk_font_chooser_dialog_new("Select Font", GTK_WINDOW(window));

    GtkResponseType result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_OK) {
        gchar *fontname = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(dialog));
        ci_display_element_set_font((CIDisplayElement*)userdata, fontname);
        g_free(fontname);
        ci_window_update();
    }

    gtk_widget_destroy(dialog);
}
