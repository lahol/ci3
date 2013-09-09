#ifndef __CI_CALL_LIST_H__
#define __CI_CALL_LIST_H__

#include <gtk/gtk.h>
#include "gtk2-compat.h"
#include <cinetmsgs.h>
#include "ci-utils.h"

typedef struct _CICallListColumn CICallListColumn;
typedef struct _CICallListLine   CICallListLine;

typedef void (*CICallListReloadFunc)(gint min_id, gint count);

void ci_call_list_set_reload_func(CICallListReloadFunc func);
void ci_call_list_set_format_func(CIFormatCallback func);

gchar *ci_call_list_get_font(void);
void ci_call_list_set_font(const gchar *font);

void ci_call_list_get_color(GdkRGBA *color);
void ci_call_list_set_color(GdkRGBA *color);

guint ci_call_list_get_line_count(void);
void ci_call_list_set_line_count(guint count);

void ci_call_list_reload(void);
void ci_call_list_update_lines(void);
void ci_call_list_set_call(guint index, CICallInfo *call);
CICallInfo *ci_call_list_get_call(guint index);

void ci_call_list_set_item_count(guint count);
guint ci_call_list_get_item_count(void);

void ci_call_list_set_offset(guint offset);
guint ci_call_list_get_offset(void);

void ci_call_list_scroll(gint count);
void ci_call_list_scroll_page(gint count);
void ci_call_list_scroll_head(void);
void ci_call_list_scroll_tail(void);

gboolean ci_call_list_get_from_pos(gdouble x, gdouble y, guint *line, guint *column);

GList *ci_call_list_get_columns(void);
CICallListColumn *ci_call_list_get_column(guint index);
CICallListColumn *ci_call_list_append_column(void);

void ci_call_list_set_column_format(CICallListColumn *column, const gchar *format);
gchar *ci_call_list_get_column_format(CICallListColumn *column);

void ci_call_list_set_column_width(CICallListColumn *column, gdouble width);
gdouble ci_call_list_get_column_width(CICallListColumn *column);

void ci_call_list_column_free_index(guint index);
void ci_call_list_column_free(CICallListColumn *column);
void ci_call_list_cleanup(void);

void ci_call_list_render(cairo_t *cr, gint left, gint bottom);

#endif
