#include "ci-window.h"
#include "ci-menu.h"
#include "ci-display-elements.h"
#include <glib/gprintf.h>
#include <memory.h>
#include "ci-config.h"
#include "ci-call-list.h"

GtkWidget *window = NULL;
GtkWidget *darea;

gint win_x = 0;
gint win_y = 0;
gint win_w = 0;
gint win_h = 0;

CIWindowMode window_mode = CIWindowModeNormal;

GdkRGBA background_color = { 1.0, 1.0, 1.0, 1.0 };

struct {
    gint start_x;
    gint start_y;
    GList *dragged_elements;
    CICallListColumn *list_column;
    gdouble start_width;
} drag_state;

gboolean ci_window_event_draw(GtkWidget *widget,
        cairo_t *cr, gpointer userdata)
{
    guint width, height;

    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);

    cairo_rectangle(cr, 0, 0, width, height);
    gdk_cairo_set_source_rgba(cr, &background_color);

    cairo_fill(cr);

    ci_display_element_render_all(cr);

    ci_call_list_render(cr, 0, win_h);

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
    win_w = event->width;
    win_h = event->height;

    ci_config_set("window:x", GINT_TO_POINTER(win_x));
    ci_config_set("window:y", GINT_TO_POINTER(win_y));
    ci_config_set("window:width", GINT_TO_POINTER(win_w));
    ci_config_set("window:height", GINT_TO_POINTER(win_h));

    ci_window_update();
    return FALSE;
}

gboolean ci_window_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer userdata)
{
    CIDisplayElement *el = ci_display_element_get_from_pos(event->x, event->y);
    guint line, column;
    gboolean inlist = ci_call_list_get_from_pos(event->x, event->y, &line, &column);

    GtkWidget *ctx_menu = NULL;

    if (event->button == 3) {
        CIDisplayContext *ctx = g_malloc0(sizeof(CIDisplayContext));
        if (el) {
            ctx->type = CIDisplayContextDisplayElement;
            ctx->data[0] = el;
        }
        else if (inlist) {
            ctx->type = CIDisplayContextList;
            ctx->data[0] = GUINT_TO_POINTER(line);
            ctx->data[1] = GUINT_TO_POINTER(column);
        }
        else {
            ctx->type = CIDisplayContextNone;
        }
        ctx_menu = ci_menu_context_menu(ctx);

        gtk_widget_show_all(ctx_menu);
        gtk_menu_popup(GTK_MENU(ctx_menu), NULL, NULL, NULL, NULL, event->button, event->time);
    }
    if (event->button == 1 && window_mode == CIWindowModeEdit/*event->state & GDK_SHIFT_MASK*/) {
        if (el) {
            drag_state.dragged_elements = g_list_prepend(drag_state.dragged_elements, (gpointer)el);
            drag_state.start_x = event->x;
            drag_state.start_y = event->y;
            drag_state.list_column = NULL;

            ci_display_element_drag_begin(el);
        }
        else if (inlist && column > 0) {
            drag_state.start_x = event->x;
            drag_state.start_y = event->y;
            drag_state.list_column = ci_call_list_get_column(column - 1);
            drag_state.start_width = ci_call_list_get_column_width(drag_state.list_column);
        }
    }

    return TRUE;
}

gboolean ci_window_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer userdata)
{
    GList *tmp;
    if ((event->state & GDK_BUTTON1_MASK) && window_mode == CIWindowModeEdit/*(event->state & GDK_SHIFT_MASK)*/) {
        gdouble dx = (gdouble)(event->x - drag_state.start_x);
        gdouble dy = (gdouble)(event->y - drag_state.start_y);

        if (drag_state.dragged_elements) {
            tmp = drag_state.dragged_elements;
            while (tmp) {
                ci_display_element_drag_update((CIDisplayElement*)tmp->data, dx, dy);
                tmp = g_list_next(tmp);
            }
        }
        else if (drag_state.list_column) {
            ci_call_list_set_column_width(drag_state.list_column, drag_state.start_width + dx);
        }
        ci_window_update();
    }
    return TRUE;
}

gboolean ci_window_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer userdata)
{
    GList *tmp;
    if (event->button == 1 && window_mode == CIWindowModeEdit/*(event->state & GDK_SHIFT_MASK)*/) {
        gdouble dx = (gdouble)(event->x - drag_state.start_x);
        gdouble dy = (gdouble)(event->y - drag_state.start_y);

        if (drag_state.dragged_elements) {
            tmp = drag_state.dragged_elements;
            while (tmp) {
                ci_display_element_drag_update((CIDisplayElement*)tmp->data, dx, dy);
                ci_display_element_drag_finish((CIDisplayElement*)tmp->data);
                tmp = g_list_next(tmp);
            }
            g_list_free(drag_state.dragged_elements);
            drag_state.dragged_elements = NULL;
        }
        else if (drag_state.list_column) {
            ci_call_list_set_column_width(drag_state.list_column, drag_state.start_width + dx);

            drag_state.list_column = NULL;
        }
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
/*    switch (event->keyval) {
        case GDK_KEY_Shift_L:
        case GDK_KEY_Shift_R:
        case GDK_KEY_Caps_Lock:
            if ((event->state & GDK_BUTTON1_MASK) && (event->state & GDK_SHIFT_MASK)) {*/
                /* cancel drag *//*
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
    }*/
    return TRUE;
}

gboolean ci_window_scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer userdata)
{
    if (event->direction == GDK_SCROLL_UP)
        ci_call_list_scroll(-1);
    else if (event->direction == GDK_SCROLL_DOWN)
        ci_call_list_scroll(1);

    ci_window_update();
    return TRUE;
}

gboolean ci_window_init(void)
{
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (window == NULL)
        return FALSE;

    ci_config_get("window:x", (gpointer)&win_x);
    ci_config_get("window:y", (gpointer)&win_y);
    ci_config_get("window:width", (gpointer)&win_w);
    ci_config_get("window:height", (gpointer)&win_h);
    ci_config_get("window:background", (gpointer)&background_color);

    gtk_window_set_focus_on_map(GTK_WINDOW(window), FALSE);
    gtk_window_set_default_size(GTK_WINDOW(window), win_w, win_h);

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

void ci_window_set_mode(CIWindowMode mode)
{
    window_mode = mode;
}

CIWindowMode ci_window_get_mode(void)
{
    return window_mode;
}

void ci_window_set_background_color(GdkRGBA *color)
{
    if (color) {
        memcpy(&background_color, color, sizeof(GdkRGBA));
        ci_config_set("window:background", color);
    }
}

gboolean ci_window_is_visible(void)
{
    return (window && gtk_widget_get_visible(window));
}

gboolean ci_window_select_font_dialog(gchar **fontname)
{
    GtkWidget *dialog = gtk_font_chooser_dialog_new("Select Font", GTK_WINDOW(window));

    if (fontname && *fontname)
        gtk_font_chooser_set_font(GTK_FONT_CHOOSER(dialog), *fontname);
    else
        gtk_font_chooser_set_font(GTK_FONT_CHOOSER(dialog), "Sans Bold 10");

    GtkResponseType result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_OK) {
        if (fontname) {
            g_free(*fontname);
            *fontname = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(dialog));
        }
    }

    gtk_widget_destroy(dialog);

    return (result == GTK_RESPONSE_OK);
}

gboolean ci_window_edit_element_dialog(gchar **format)
{
    if (format == NULL)
        return FALSE;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Edit Format String",
            GTK_WINDOW(window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_STOCK_APPLY,
            GTK_RESPONSE_APPLY,
            GTK_STOCK_CANCEL,
            GTK_RESPONSE_CANCEL,
            NULL);

    GtkWidget *entry, *content_area;
    GtkEntryBuffer *buffer = gtk_entry_buffer_new(*format, -1);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    entry = gtk_entry_new_with_buffer(buffer);

    gtk_widget_show(entry);

    gtk_container_add(GTK_CONTAINER(content_area), entry);

    GtkResponseType result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_APPLY) {
        g_free(*format);
        *format = g_strdup(gtk_entry_buffer_get_text(buffer));
    }

    g_object_unref(G_OBJECT(buffer));
    gtk_widget_destroy(dialog);

    return (result == GTK_RESPONSE_APPLY);
}

gboolean ci_window_choose_color_dialog(GdkRGBA *color)
{
    GtkWidget *dialog = gtk_color_chooser_dialog_new("Select Color", GTK_WINDOW(window));

    GtkResponseType result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_OK) {
        if (color) {
            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dialog), color);
        }
    }

    gtk_widget_destroy(dialog);

    return (result == GTK_RESPONSE_OK);
}

