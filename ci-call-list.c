#include "ci-call-list.h"
#include <cinet.h>
#include <glib/gprintf.h>

struct _CICallListColumn {
    guint index;
    gdouble width;
    gchar *format;
};

struct _CICallListLine {
    guint index;
    CICallInfo call;
    GList *contents;
};

struct {
    GList *columns;
    GList *lines;
    gchar *font;
    guint linecount;
    gdouble lineheight;
    guint itemcount;
    guint offset;
    CICallListReloadFunc reload_func;
    CIFormatCallback format_func;
} ci_call_list;

void ci_call_list_free_line(CICallListLine *line);

void ci_call_list_set_reload_func(CICallListReloadFunc func)
{
    ci_call_list.reload_func = func;
}

void ci_call_list_set_format_func(CIFormatCallback func)
{
    ci_call_list.format_func = func;
}

void ci_call_list_set_line_count(guint count)
{
    ci_call_list.linecount = count;
    ci_call_list.lines = ci_util_list_truncate(ci_call_list.lines, count,
            (GFreeFunc)ci_call_list_free_line);

    guint len = g_list_length(ci_call_list.lines);
    GList *app = NULL;
    CICallListLine *line;
    if (len < count) {
        for (; len < count; ++len) {
            line = g_malloc0(sizeof(CICallListLine));
            app = g_list_prepend(app, line);
        }
        ci_call_list.lines = g_list_concat(ci_call_list.lines, app);
    }
}

void ci_call_list_format_line(CICallListLine *line)
{
    if (line == NULL)
        return;
    g_list_free_full(line->contents, g_free);
    line->contents = NULL;

    GList *col = g_list_last(ci_call_list.columns);
    gchar *content;
    while (col) {
        content = ci_util_format_string(((CICallListColumn*)col->data)->format,
                ci_call_list.format_func, &(line->call));
        line->contents = g_list_prepend(line->contents, content);
        col = g_list_previous(col);
    }
}

void ci_call_list_set_call(guint index, CICallInfo *call)
{
    CICallListLine *line = (CICallListLine*)g_list_nth_data(ci_call_list.lines, index);
    if (line) {
        cinet_call_info_copy(&line->call, call);
        ci_call_list_format_line(line);
    }
}

void ci_call_list_set_item_count(guint count)
{
    ci_call_list.itemcount = count;
}

void ci_call_list_set_offset(guint offset)
{
    if (offset + ci_call_list.linecount > ci_call_list.itemcount) {
        if (ci_call_list.linecount > ci_call_list.itemcount)
            ci_call_list.offset = 0;
        else
            ci_call_list.offset = ci_call_list.itemcount - ci_call_list.linecount;
    }
    else
        ci_call_list.offset = offset;

    if (ci_call_list.reload_func)
        ci_call_list.reload_func(ci_call_list.offset, ci_call_list.linecount);
}

guint ci_call_list_get_offset(void)
{
    return ci_call_list.offset;
}

void ci_call_list_scroll(gint count)
{
    if (count == 0)
        return;
    guint offset = ci_call_list.offset;
    if (count > 0)
        offset += count;
    else if (ci_call_list.offset > -count)
        offset += count;
    else
        offset = 0;
    ci_call_list_set_offset(offset);
}

gboolean ci_call_list_get_from_pos(gdouble x, gdouble y, guint *line, guint *column)
{
    return FALSE;
}

CICallListColumn *ci_call_list_get_column(guint index)
{
    return NULL;
}

CICallListColumn *ci_call_list_append_column(void)
{
    CICallListColumn *column = g_malloc0(sizeof(CICallListColumn));
    column->index = g_list_length(ci_call_list.columns);
    ci_call_list.columns = g_list_append(ci_call_list.columns, column);
    return column;
}

void ci_call_list_set_column_format(CICallListColumn *column, const gchar *format)
{
    if (column) {
        g_free(column->format);
        column->format = g_strdup(format);
    }
}

void ci_call_list_set_column_width(CICallListColumn *column, gdouble width)
{
    if (column != NULL)
        column->width = width;
}

void ci_call_list_column_free(guint index)
{
}

void ci_call_list_free_line(CICallListLine *line)
{
    if (line == NULL)
        return;
    g_list_free_full(line->contents, g_free);
    cinet_call_info_free(&line->call);
    g_free(line);
}

void ci_call_list_free_column(CICallListColumn *col)
{
    if (col == NULL)
        return;
    g_free(col->format);
    g_free(col);
}

void ci_call_list_cleanup(void)
{
    g_list_free_full(ci_call_list.columns, (GFreeFunc)ci_call_list_free_column);
    g_list_free_full(ci_call_list.lines, (GFreeFunc)ci_call_list_free_line);
}

void ci_call_list_render_line(cairo_t *cr, PangoLayout *layout, gdouble x, gdouble y,
                              CICallListLine *line)
{
    GList *col = ci_call_list.columns;
    GList *lcol = line->contents;

    cairo_identity_matrix(cr);
    cairo_translate(cr, x, y);

    while (col && lcol) {
        cairo_rectangle(cr, 0.0, 0.0, ((CICallListColumn*)col->data)->width, ci_call_list.lineheight);
        cairo_clip(cr);

        pango_layout_set_text(layout, (gchar*)lcol->data, -1);
        pango_cairo_update_layout(cr, layout);
        pango_cairo_show_layout(cr, layout);

        cairo_reset_clip(cr);

        x += ((CICallListColumn*)col->data)->width;

        col = g_list_next(col);
        lcol = g_list_next(lcol);
    }
}

void ci_call_list_render(cairo_t *cr, gint left, gint bottom)
{
    PangoLayout *layout;
    PangoFontDescription *fdesc;
    int w, h;

    layout = pango_cairo_create_layout(cr);

    if (ci_call_list.font)
        fdesc = pango_font_description_from_string(ci_call_list.font);
    else
        fdesc = pango_font_description_from_string("Sans Bold 10");

    pango_layout_set_font_description(layout, fdesc);
    pango_font_description_free(fdesc);

    pango_layout_set_text(layout, "Dummy", -1);
    pango_layout_get_size(layout, &w, &h);

    ci_call_list.lineheight = ((gdouble)h)/PANGO_SCALE;

    gdouble start = bottom - ci_call_list.linecount * ci_call_list.lineheight;

    GList *tmp;
    guint index;

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    
    for (tmp = ci_call_list.lines, index = 0;
         tmp != NULL && index < ci_call_list.linecount;
         tmp = g_list_next(tmp), ++index) {
        ci_call_list_render_line(cr, layout, left, start + index * ci_call_list.lineheight,
                (CICallListLine*)tmp->data);
    }

    g_object_unref(layout);
}
