#ifndef __CI_DATA_H__
#define __CI_DATA_H__

#include <glib.h>
#include <cinetmsgs.h>

typedef struct {
    gchar *completenumber;
    gchar *areacode;
    gchar *number;
    gchar *date;
    gchar *time;
    gchar *msn;
    gchar *alias;
    gchar *area;
    gchar *name;
} CIDataSet;

void ci_data_set_free(CIDataSet *set);
void ci_data_set_from_ring_event(CIDataSet *set, CINetMsgEventRing *msg, gboolean overwrite);

#endif
