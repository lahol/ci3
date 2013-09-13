#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "ci-logging.h"
#include "ci-config.h"

gchar filename[1024];
gboolean initialized = FALSE;

void ci_log(gchar *format, ...)
{
    if (!initialized) {
        gchar *fname = ci_config_get_string("general:log-file");
        if (fname)
            strcpy(filename, fname);
        else
            filename[0] = 0;
        g_free(fname);
        initialized = TRUE;
    }

    if (filename[0] == 0)
        return;

    FILE *f;

    if ((f = fopen(filename, "a")) == NULL)
        return;

    time_t t;
    static char buf[64];
    static struct tm bdt;

    time(&t);
    localtime_r(&t, &bdt);
    strftime(buf, 63, "[%Y%m%d-%H%M%S] ", &bdt);
    fputs(buf, f);

    va_list args;
    va_start(args, format);
    vfprintf(f, format, args);
    va_end(args);

    fclose(f);
}

