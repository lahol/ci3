#ifndef __CI_CALL_LIST_H__
#define __CI_CALL_LIST_H__

#include <gtk/gtk.h>
#include <cinetmsgs.h>
#include "ci-utils.h"

typedef struct _CICallListColumn CICallListColumn;
typedef struct _CICallListLine   CICallListLine;

typedef void (*CICallListReloadFunc)(gint min_id, gint count);

void ci_call_list_set_reload_func(CICallListReloadFunc func);
void ci_call_list_set_format_func(CIFormatCallback func);

void ci_call_list_set_line_count(guint count);
void ci_call_list_set_call(guint index, CICallInfo *call);

void ci_call_list_set_item_count(guint count);

void ci_call_list_set_offset(guint offset);
guint ci_call_list_get_offset(void);

void ci_call_list_scroll(gint count);

gboolean ci_call_list_get_from_pos(gdouble x, gdouble y, guint *line, guint *column);

CICallListColumn *ci_call_list_get_column(guint index);
CICallListColumn *ci_call_list_append_column(void);
void ci_call_list_set_column_format(CICallListColumn *column, const gchar *format);
void ci_call_list_set_column_width(CICallListColumn *column, gdouble width);

void ci_call_list_column_free(guint index);
void ci_call_list_cleanup(void);

void ci_call_list_render(cairo_t *cr, gint left, gint bottom);

#endif
