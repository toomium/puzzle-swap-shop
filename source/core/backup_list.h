#ifndef PSS_CORE_BACKUP_LIST_H
#define PSS_CORE_BACKUP_LIST_H

#include <stdbool.h>
#include <stddef.h>

#include "core/manifest.h"

/*
 * Texts of one row of S7 "Sicherungen", made
 * from the folder name and the manifest in the current language. No file
 * access here; the screen lists the backups through the model.
 */

/*
 * What the backup was made for: "Ursicherung", "vor Kauf: Sommerwiese, 3 Teile",
 * "vor Wiederherstellung". `panel_name` is the name of the purchased panel,
 * or NULL if it is not known; the purchase then reads "vor Kauf".
 */
void backup_row_reason(const char *name, const Manifest *m, const char *panel_name, char *out,
                       size_t size);

/*
 * Date and time for the row. Dated backups use their name, the original its
 * manifest. Day first in German ("26.10.2026, 18:04"), else "2026-10-26 18:04".
 * False if the backup has no valid time.
 */
bool backup_row_date(const char *name, const Manifest *m, bool day_first, char *out, size_t size);

/* The original backup cannot be removed or replaced (lock symbol). */
bool backup_row_locked(const char *name);

#endif
