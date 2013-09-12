#ifndef __CI_FILTER_H__
#define __CI_FILTER_H__

#include <glib.h>

void ci_filter_msn_set(const gchar *filter_string);
gboolean ci_filter_msn_allowed(const gchar *msn);

void ci_filter_cleanup(void);

#endif
