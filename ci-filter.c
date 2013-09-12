#include "ci-filter.h"
#include <glib/gprintf.h>

GList *allowed_msn = NULL;

void ci_filter_msn_list_clear(void)
{
    g_list_free_full(allowed_msn, g_free);
    allowed_msn = NULL;
}

void ci_filter_msn_set(const gchar *filter_string)
{
    ci_filter_msn_list_clear();

    if (filter_string == NULL)
        return;

    gchar *tmp = g_strstrip(g_strdup(filter_string));
    if (tmp[0] == 0 || g_strcmp0(tmp, "all") == 0) {
        g_free(tmp);
        return;
    }

    /* check that string contains only valid characters, i.e. digits, whitespaces
     * and some reasonable delimiters: ,;|: */
    gsize offset, len;
    for (offset = 0; tmp[offset] != 0; ++offset)
        if (!g_ascii_isdigit(tmp[offset]) &&
                !g_ascii_isspace(tmp[offset]) &&
                tmp[offset] != ',' && tmp[offset] != ':' &&
                tmp[offset] != ';' && tmp[offset] != '|') {
            g_printf("msnlist: illegal character: %c\n", tmp[offset]);
            g_free(tmp);
            return;
        }


    offset = 0;
    while (tmp[offset] != 0) {
        /* strip leading non-digits */
        while (!g_ascii_isdigit(tmp[offset])) ++offset;
        for (len = 0; g_ascii_isdigit(tmp[offset + len]); ++len);
        if (len > 0) {
            allowed_msn = g_list_prepend(allowed_msn, g_strndup(&tmp[offset], len));
            offset += len;
        }
    }
}

gboolean ci_filter_msn_allowed(const gchar *msn)
{
    if (allowed_msn == NULL)
        return TRUE;

    GList *tmp;
    for (tmp = allowed_msn; tmp != NULL; tmp = g_list_next(tmp))
        if (g_strcmp0((gchar*)tmp->data, msn) == 0)
            return TRUE;

    return FALSE;
}

void ci_filter_cleanup(void)
{
    ci_filter_msn_list_clear();
}
