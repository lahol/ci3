#ifndef __CI_NOTIFY_H__
#define __CI_NOTIFY_H__

#include <glib.h>
#include <cinetmsgs.h>

void ci_notify_present(CINetMsgType type, CICallInfo *call_info, gchar *msgid);
void ci_notify_cleanup(void);

#endif
