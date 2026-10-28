#include "core/backup_list.h"

#include <stdio.h>

#include "core/strings.h"

void backup_row_reason(const char *name, const Manifest *m, const char *panel_name, char *out,
                       size_t size)
{
    if (backup_name_is_original(name) || m->reason == BACKUP_REASON_ORIGINAL) {
        snprintf(out, size, "%s", str(STR_BACKUP_ORIGINAL));
        return;
    }
    switch (m->reason) {
    case BACKUP_REASON_PURCHASE:
        if (m->has_purchase && panel_name && m->pieces > 0) {
            char pieces[24];
            strings_pieces(pieces, sizeof(pieces), m->pieces);
            snprintf(out, size, str(STR_BACKUP_BEFORE_PURCHASE), panel_name, pieces);
        } else {
            snprintf(out, size, "%s", str(STR_BACKUP_BEFORE_PURCHASE_PLAIN));
        }
        break;
    case BACKUP_REASON_RESTORE:
        snprintf(out, size, "%s", str(STR_BACKUP_BEFORE_RESTORE));
        break;
    default:
        snprintf(out, size, "%s", str(STR_BACKUP_BEFORE_CHANGE));
        break;
    }
}

bool backup_row_date(const char *name, const Manifest *m, bool day_first, char *out, size_t size)
{
    if (backup_name_label(name, day_first, out, size))
        return true;
    char full[20];
    if (!timestamp_format(&m->created, full))
        return false;
    /* "2026-10-26 19:24:00": the name-based label drops the seconds, so does this. */
    if (day_first)
        snprintf(out, size, "%.2s.%.2s.%.4s, %.5s", full + 8, full + 5, full, full + 11);
    else
        snprintf(out, size, "%.16s", full);
    return true;
}

bool backup_row_locked(const char *name)
{
    return backup_name_is_original(name);
}
