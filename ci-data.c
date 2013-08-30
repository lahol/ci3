#include "ci-data.h"
#include <memory.h>

void ci_data_set_free(CIDataSet *set)
{
    if (!set)
        return;
    g_free(set->completenumber);
    g_free(set->areacode);
    g_free(set->number);
    g_free(set->date);
    g_free(set->time);
    g_free(set->msn);
    g_free(set->alias);
    g_free(set->area);
    g_free(set->name);

    memset(set, 0, sizeof(CIDataSet));
}

void ci_data_set_from_ring_event(CIDataSet *set, CINetMsgEventRing *msg, gboolean overwrite)
{
    if (overwrite)
        ci_data_set_free(set);
    if (!msg || !set)
        return;
#define SET_ENTR(entr, flag) do {\
    if (msg->fields & flag) {\
        if (set->entr) g_free(set->entr);\
        set->entr = g_strdup(msg->entr);\
    }} while (0)

    SET_ENTR(completenumber, CIF_COMPLETENUMBER);
    SET_ENTR(areacode, CIF_AREACODE);
    SET_ENTR(number, CIF_NUMBER);
    SET_ENTR(date, CIF_DATE);
    SET_ENTR(time, CIF_TIME);
    SET_ENTR(msn, CIF_MSN);
    SET_ENTR(alias, CIF_ALIAS);
    SET_ENTR(area, CIF_AREA);
    SET_ENTR(name, CIF_NAME);
    
#undef SET_ENTR
}
