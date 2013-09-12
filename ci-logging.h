#ifndef __CI_LOGGING_H__
#define __CI_LOGGING_H__

#include <glib.h>

void ci_log(gchar *format, ...);

#ifdef DEBUG
#define DLOG(fmt, ...) ci_log("*DEBUG* " fmt, ##__VA_ARGS__)
#else
#define DLOG(fmt, ...)
#endif

#define LOG(fmt, ...) ci_log(fmt, ##__VA_ARGS__)

#endif
