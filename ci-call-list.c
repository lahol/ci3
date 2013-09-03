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
    guint linecount;
    gdouble lineheight;
    guint itemcount;
    guint offset;
    CICallListReloadFunc reload_func;
} ci_call_list;

void ci_call_list_set_reload_func(CICallListReloadFunc func)
{
    ci_call_list.reload_func = func;
}

void ci_call_list_set_line_count(guint count)
{
    ci_call_list.linecount = count;
}

void ci_call_list_set_call(guint index, CICallInfo *call)
{
    if (call) {
        g_printf("set call %u: [%03d] %s %s (%s) %s %s an %s %s\n",
                index, call->id, call->date, call->time, call->areacode, call->number,
                call->name, call->msn, call->alias);
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
    return NULL;
}

void ci_call_list_set_column_format(CICallListColumn *column, const gchar *format)
{
}

void ci_call_list_set_column_width(CICallListColumn *column, gdouble width)
{
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

void ci_call_list_render(cairo_t *cr, gdouble left, gdouble bottom)
{
}
