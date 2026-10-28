#include "core/backup_names.h"

#include <stdio.h>
#include <string.h>

static unsigned days_in_month(unsigned year, unsigned month)
{
    static const unsigned char days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0))
        return 29;
    return days[month - 1];
}

bool timestamp_valid(const Timestamp *t)
{
    return t->year >= 2000 && t->year <= 9999 && t->month >= 1 && t->month <= 12 && t->day >= 1 &&
           t->day <= days_in_month(t->year, t->month) && t->hour <= 23 && t->minute <= 59 &&
           t->second <= 59;
}

/* Writes v as exactly `width` decimal digits; callers pass validated values. */
static char *put_digits(char *p, unsigned v, unsigned width)
{
    for (unsigned i = width; i > 0; i--) {
        p[i - 1] = (char)('0' + v % 10);
        v /= 10;
    }
    return p + width;
}

bool timestamp_format(const Timestamp *t, char out[20])
{
    if (!timestamp_valid(t))
        return false;
    char *p = put_digits(out, t->year, 4);
    *p++ = '-';
    p = put_digits(p, t->month, 2);
    *p++ = '-';
    p = put_digits(p, t->day, 2);
    *p++ = ' ';
    p = put_digits(p, t->hour, 2);
    *p++ = ':';
    p = put_digits(p, t->minute, 2);
    *p++ = ':';
    p = put_digits(p, t->second, 2);
    *p = '\0';
    return true;
}

static bool digits(const char *s, unsigned n, unsigned *out)
{
    unsigned v = 0;
    for (unsigned i = 0; i < n; i++) {
        if (s[i] < '0' || s[i] > '9')
            return false;
        v = v * 10 + (unsigned)(s[i] - '0');
    }
    *out = v;
    return true;
}

bool timestamp_parse(const char *s, Timestamp *t)
{
    unsigned y, mo, d, h, mi, se;
    if (strlen(s) != 19 || s[4] != '-' || s[7] != '-' || s[10] != ' ' || s[13] != ':' ||
        s[16] != ':')
        return false;
    if (!digits(s, 4, &y) || !digits(s + 5, 2, &mo) || !digits(s + 8, 2, &d) ||
        !digits(s + 11, 2, &h) || !digits(s + 14, 2, &mi) || !digits(s + 17, 2, &se))
        return false;
    Timestamp r = {(uint16_t)y, (uint8_t)mo, (uint8_t)d, (uint8_t)h, (uint8_t)mi, (uint8_t)se};
    if (!timestamp_valid(&r))
        return false;
    *t = r;
    return true;
}

bool timestamp_from_unix(int64_t secs, Timestamp *t)
{
    if (secs < 0)
        return false;
    /* Civil-from-days by Howard Hinnant, days counted from 0000-03-01. */
    int64_t z = secs / 86400 + 719468;
    unsigned rem = (unsigned)(secs % 86400);
    int64_t era = z / 146097;
    unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    unsigned mp = (5 * doy + 2) / 153;
    unsigned day = doy - (153 * mp + 2) / 5 + 1;
    unsigned month = mp < 10 ? mp + 3 : mp - 9;
    int64_t year = era * 400 + yoe + (month <= 2);
    if (year < 2000 || year > 9999)
        return false;
    Timestamp r = {(uint16_t)year,        (uint8_t)month,           (uint8_t)day,
                   (uint8_t)(rem / 3600), (uint8_t)(rem / 60 % 60), (uint8_t)(rem % 60)};
    *t = r;
    return true;
}

bool backup_name_make(const Timestamp *t, unsigned suffix, char out[BACKUP_NAME_MAX])
{
    if (!timestamp_valid(t) || suffix == 1 || suffix > BACKUP_MAX_SUFFIX)
        return false;
    char *p = put_digits(out, t->year, 4);
    p = put_digits(p, t->month, 2);
    p = put_digits(p, t->day, 2);
    *p++ = '-';
    p = put_digits(p, t->hour, 2);
    p = put_digits(p, t->minute, 2);
    p = put_digits(p, t->second, 2);
    if (suffix > 1) {
        *p++ = '-';
        p = put_digits(p, suffix, 1);
    }
    *p = '\0';
    return true;
}

bool backup_name_is_original(const char *name)
{
    return strcmp(name, BACKUP_ORIGINAL) == 0;
}

bool backup_name_valid(const char *name)
{
    if (backup_name_is_original(name))
        return true;
    size_t n = strlen(name);
    if (n != 15 && n != 17)
        return false;
    unsigned date, time;
    if (!digits(name, 8, &date) || name[8] != '-' || !digits(name + 9, 6, &time))
        return false;
    Timestamp t = {(uint16_t)(date / 10000), (uint8_t)(date / 100 % 100), (uint8_t)(date % 100),
                   (uint8_t)(time / 10000),  (uint8_t)(time / 100 % 100), (uint8_t)(time % 100)};
    if (!timestamp_valid(&t))
        return false;
    if (n == 17)
        return name[15] == '-' && name[16] >= '2' && name[16] <= '0' + BACKUP_MAX_SUFFIX;
    return true;
}

bool backup_name_label(const char *name, bool day_first, char *out, size_t size)
{
    if (backup_name_is_original(name) || !backup_name_valid(name))
        return false;
    unsigned date, time;
    if (!digits(name, 8, &date) || !digits(name + 9, 6, &time))
        return false;
    unsigned y = date / 10000, mo = date / 100 % 100, d = date % 100;
    unsigned h = time / 10000, mi = time / 100 % 100;
    int n = day_first ? snprintf(out, size, "%02u.%02u.%04u, %02u:%02u", d, mo, y, h, mi)
                      : snprintf(out, size, "%04u-%02u-%02u %02u:%02u", y, mo, d, h, mi);
    return n > 0 && (size_t)n < size;
}

unsigned backup_prune_select(const char *const *names, unsigned count, unsigned keep, bool *remove)
{
    unsigned marked = 0;
    for (unsigned i = 0; i < count; i++) {
        remove[i] = false;
        if (!backup_name_valid(names[i]) || backup_name_is_original(names[i]))
            continue;
        /* Number of valid, non-original names that are newer than this one. */
        unsigned newer = 0;
        for (unsigned j = 0; j < count; j++) {
            if (j == i || !backup_name_valid(names[j]) || backup_name_is_original(names[j]))
                continue;
            int c = strcmp(names[j], names[i]);
            if (c > 0 || (c == 0 && j < i))
                newer++;
        }
        if (newer >= keep) {
            remove[i] = true;
            marked++;
        }
    }
    return marked;
}
