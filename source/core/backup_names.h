#ifndef PSS_CORE_BACKUP_NAMES_H
#define PSS_CORE_BACKUP_NAMES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Names of backup folders under backups/.
 * "000-original" is the first backup and is never removed. All others are
 * named after their creation time, "YYYYMMDD-HHMMSS", with "-2" to "-9"
 * appended when a name is taken. Sorting names as strings sorts them by age.
 */

#define BACKUP_NAME_MAX   20 /* including NUL */
#define BACKUP_ORIGINAL   "000-original"
#define BACKUP_MAX_SUFFIX 9

typedef struct {
    uint16_t year;
    uint8_t month;  /* 1..12 */
    uint8_t day;    /* 1..31 */
    uint8_t hour;   /* 0..23 */
    uint8_t minute; /* 0..59 */
    uint8_t second; /* 0..59 */
} Timestamp;

bool timestamp_valid(const Timestamp *t);

/* "2026-10-26 19:24:00"; out must hold 20 bytes. False if t is invalid. */
bool timestamp_format(const Timestamp *t, char out[20]);
bool timestamp_parse(const char *s, Timestamp *t);

/*
 * Seconds since 1970-01-01 00:00:00 to a calendar time, without time zones.
 * On the console time() counts in the local time of the system clock, so this
 * gives what the user sees in the system settings. False outside 2000..9999.
 */
bool timestamp_from_unix(int64_t secs, Timestamp *t);

/* "20261026-192400" or with suffix 2..9 "20261026-192400-2". */
bool backup_name_make(const Timestamp *t, unsigned suffix, char out[BACKUP_NAME_MAX]);

/*
 * True only for names this app creates. Names come from directory listings
 * and the journal, so this also keeps paths free of '/' and "..".
 */
bool backup_name_valid(const char *name);

bool backup_name_is_original(const char *name);

/*
 * Date and time of a dated backup for the screens: "26.10.2026, 18:04" with
 * day_first, else "2026-10-26 18:04". False for the original, which has no
 * date in its name, and for invalid names.
 */
bool backup_name_label(const char *name, bool day_first, char *out, size_t size);

/*
 * Marks which backups to delete so that at most `keep` remain besides the
 * original. names may be in any order; remove[i] is set for names[i]. Invalid
 * names and the original are never marked. Returns the number marked.
 */
unsigned backup_prune_select(const char *const *names, unsigned count, unsigned keep, bool *remove);

#endif
