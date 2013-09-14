#include "ci-window.h"
#include "ci-menu.h"
#include "ci-display-elements.h"
#include <memory.h>
#include "ci-config.h"
#include "ci-call-list.h"
#include <glib/gi18n.h>

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

#if !GTK_CHECK_VERSION(3,0,0)
gboolean ci_window_expose_event(GtkWidget *widget, GdkEvent *event, gpointer userdata)
{
    cairo_t *cr = gdk_cairo_create(widget->window);
    gboolean res = ci_window_event_draw(widget, cr, userdata);
    cairo_destroy(cr);

    return res;
}
#endif

gboolean ci_window_focus(GtkWidget *widget, GtkDirectionType dir, gpointer userdata)
{
/*    gtk_window_set_urgency_hint(GTK_WINDOW(window), FALSE);*/
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

    ci_config_set_int("window:x", win_x);
    ci_config_set_int("window:y", win_y);
    ci_config_set_int("window:width", win_w);
    ci_config_set_int("window:height", win_h);

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
            ctx->type = CIContextTypeDisplayElement;
            ctx->data[0] = el;
        }
        else if (inlist) {
            ctx->type = CIContextTypeList;
            ctx->data[0] = GUINT_TO_POINTER(line);
            ctx->data[1] = GUINT_TO_POINTER(column);
        }
        else {
            ctx->type = CIContextTypeNone;
            ctx->data[0] = GINT_TO_POINTER(event->x);
            ctx->data[1] = GINT_TO_POINTER(event->y);
        }
        ctx_menu = ci_menu_context_menu(ctx->type, ctx);

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

        gdouble gs = 1.0;
        if (event->state & GDK_CONTROL_MASK)
            gs = 10.0;

        if (drag_state.dragged_elements) {
            tmp = drag_state.dragged_elements;
            while (tmp) {
                ci_display_element_drag_update((CIDisplayElement*)tmp->data, dx, dy, gs);
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

        gdouble gs = 1.0;
        if (event->state & GDK_CONTROL_MASK)
            gs = 10.0;

        if (drag_state.dragged_elements) {
            tmp = drag_state.dragged_elements;
            while (tmp) {
                ci_display_element_drag_update((CIDisplayElement*)tmp->data, dx, dy, gs);
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
        case GDK_KEY_Page_Up:
            ci_call_list_scroll_page(-1);
            break;
        case GDK_KEY_Page_Down:
            ci_call_list_scroll_page(1);
            break;
        case GDK_KEY_Up:
            ci_call_list_scroll(-1);
            break;
        case GDK_KEY_Down:
            ci_call_list_scroll(1);
            break;
        case GDK_KEY_Home:
            ci_call_list_scroll_head();
            break;
        case GDK_KEY_End:
            ci_call_list_scroll_tail();
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

    win_x = ci_config_get_int("window:x");
    win_y = ci_config_get_int("window:y");
    win_w = ci_config_get_int("window:width");
    win_h = ci_config_get_int("window:height");
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

#if GTK_CHECK_VERSION(3,0,0)
    g_signal_connect(G_OBJECT(darea), "draw", G_CALLBACK(ci_window_event_draw), NULL);
#else
    g_signal_connect(G_OBJECT(darea), "expose-event", G_CALLBACK(ci_window_expose_event), NULL);
#endif
    g_signal_connect(G_OBJECT(darea), "button-press-event", G_CALLBACK(ci_window_button_press_event), NULL);
    g_signal_connect(G_OBJECT(darea), "motion-notify-event", G_CALLBACK(ci_window_motion_notify_event), NULL);
    g_signal_connect(G_OBJECT(darea), "button-release-event", G_CALLBACK(ci_window_button_release_event), NULL);
    g_signal_connect(G_OBJECT(darea), "scroll-event", G_CALLBACK(ci_window_scroll_event), NULL);

    g_signal_connect(G_OBJECT(window), "focus", G_CALLBACK(ci_window_focus), NULL);
    g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(ci_window_event_delete_event), NULL);
    g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(ci_window_configure_event), NULL);
    g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(ci_window_key_press_event), NULL);
    g_signal_connect(G_OBJECT(window), "key-release-event", G_CALLBACK(ci_window_key_release_event), NULL);
     /* g_signal_connect(G_OBJECT(darea), "button-press-event", "scroll-event", "destroy" */
    gtk_window_set_role(GTK_WINDOW(window), "MainWindow");

    gtk_window_set_gravity(GTK_WINDOW(window), GDK_GRAVITY_STATIC);

    return TRUE;
}

void ci_window_show(gboolean mark_as_urgent, gboolean focus)
{
    if (window) {
        gtk_window_present(GTK_WINDOW(window));
        gtk_window_move(GTK_WINDOW(window), win_x, win_y);
/*        if (mark_as_urgent)
            gtk_window_set_urgency_hint(GTK_WINDOW(window), TRUE);*/
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
#if GTK_CHECK_VERSION(3,2,0)
    GtkWidget *dialog = gtk_font_chooser_dialog_new(NULL, GTK_WINDOW(window));

    if (fontname && *fontname)
        gtk_font_chooser_set_font(GTK_FONT_CHOOSER(dialog), *fontname);
    else
        gtk_font_chooser_set_font(GTK_FONT_CHOOSER(dialog), "Sans Bold 10");
#else
    GtkWidget *dialog = gtk_font_selection_dialog_new(NULL);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));

    if (fontname && *fontname)
        gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(dialog), *fontname);
    else
        gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(dialog), "Sans Bold 10");
#endif
    GtkResponseType result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_OK) {
        if (fontname) {
            g_free(*fontname);
#if GTK_CHECK_VERSION(3,2,0)
            *fontname = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(dialog));
#else
            *fontname = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(dialog));
#endif
        }
    }

    gtk_widget_destroy(dialog);

    return (result == GTK_RESPONSE_OK);
}

gboolean ci_window_edit_element_dialog(gchar **format)
{
    if (format == NULL)
        return FALSE;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Edit format string"),
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

    g_object_set(G_OBJECT(entry), "activates-default", TRUE, NULL);

    gtk_widget_show(entry);

    gtk_container_add(GTK_CONTAINER(content_area), entry);

    GtkWidget *default_button = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog),
                                                                   GTK_RESPONSE_APPLY);

    if (default_button != NULL) {
        gtk_widget_set_can_default(default_button, TRUE);
        gtk_widget_grab_default(default_button);
    }

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
#if GTK_CHECK_VERSION(3,2,0)
    GtkWidget *dialog = gtk_color_chooser_dialog_new(NULL, GTK_WINDOW(window));
#else
    GtkWidget *dialog = gtk_color_selection_dialog_new(NULL);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
#endif

    GtkResponseType result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_OK) {
        if (color) {
#if GTK_CHECK_VERSION(3,2,0)
            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dialog), color);
#else
            GtkWidget *cch = gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(dialog));
            GdkColor col;
            guint alpha;
            gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(cch), &col);
            alpha = gtk_color_selection_get_current_alpha(GTK_COLOR_SELECTION(cch));

            color->red = col.red/65535.0f;
            color->green = col.green/65535.0f;
            color->blue = col.blue/65535.0f;
            color->alpha = alpha/65535.0f;
#endif
        }
    }

    gtk_widget_destroy(dialog);

    return (result == GTK_RESPONSE_OK);
}

GtkWidget *ci_window_get_window(void)
{
    return window;
}
