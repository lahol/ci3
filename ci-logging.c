#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "ci-logging.h"
#include "ci-config.h"
#include <time.h>

gchar filename[1024];
gboolean initialized = FALSE;

void ci_logging_reinit(void)
{
    gchar *fname = ci_config_get_string("general:log-file");
    if (fname)
        strcpy(filename, fname);
    else
        filename[0] = 0;
    g_free(fname);
    initialized = TRUE;
}

void ci_log(gchar *format, ...)
{
    if (!initialized)
        ci_logging_reinit();

    if (filename[0] == 0)
        return;

    FILE *f;

    if ((f = fopen(filename, "a")) == NULL)
        return;

    time_t t;
    static char buf[64];
    static struct tm bdt, *pdt = NULL;

    time(&t);
#ifdef WIN32
    pdt = localtime(&t);
#else
    localtime_r(&t, &bdt);
    pdt = &bdt;
#endif
    strftime(buf, 63, "[%Y%m%d-%H%M%S] ", pdt);
    fputs(buf, f);

    va_list args;
    va_start(args, format);
    vfprintf(f, format, args);
    va_end(args);

    fclose(f);
}

