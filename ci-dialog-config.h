#ifndef __CI_DIALOG_CONFIG_H__
#define __CI_DIALOG_CONFIG_H__

#include <glib.h>

typedef void (*CIConfigChangedCallback)(void);

void ci_dialog_config_show(CIConfigChangedCallback changed_cb);

#endif
