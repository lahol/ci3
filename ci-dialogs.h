#ifndef __CI_DIALOGS_H__
#define __CI_DIALOGS_H__

#include <gtk/gtk.h>
#include <cinetmsgs.h>

#include "ci-dialog-config.h"
#include "gtk2-compat.h"

gboolean ci_dialogs_add_caller(CICallerInfo *caller);

void ci_dialogs_about(void);
gboolean ci_dialog_choose_color_dialog(gchar *title, GdkRGBA *color);

#endif
