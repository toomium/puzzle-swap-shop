#include "core/text_fit.h"

#include <string.h>

/* Longer texts are cut at this many character boundaries; names are far shorter. */
#define TEXT_FIT_MAX_CHARS 256

size_t text_utf8_len(unsigned char c)
{
    if (c >= 0xF0 && c <= 0xF4)
        return 4;
    if (c >= 0xE0)
        return c <= 0xEF ? 3 : 1;
    if (c >= 0xC2)
        return 2;
    return 1;
}

size_t text_fit(const char *s, int max_w, int ellipsis_w, TextMeasureFn measure, void *ctx,
                bool *cut)
{
    size_t len = strlen(s);
    *cut = false;
    if (measure(s, len, ctx) <= max_w)
        return len;
    *cut = true;

    /* Ends of the candidate prefixes, one per character. */
    size_t ends[TEXT_FIT_MAX_CHARS];
    int count = 0;
    for (size_t i = 0; i < len && count < TEXT_FIT_MAX_CHARS;) {
        size_t step = text_utf8_len((unsigned char)s[i]);
        if (i + step > len)
            step = len - i;
        i += step;
        ends[count++] = i;
    }

    /* Binary search for the longest prefix that fits next to the ellipsis. */
    int lo = 0, hi = count - 1, best = -1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (measure(s, ends[mid], ctx) + ellipsis_w <= max_w) {
            best = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    if (best < 0)
        return 0;

    size_t n = ends[best];
    while (n > 0 && s[n - 1] == ' ')
        n--;
    return n;
}

/* Longest prefix of s[0..len) at a character boundary that fits max_w; at least one character. */
static size_t fit_chars(const char *s, size_t len, int max_w, TextMeasureFn measure, void *ctx)
{
    size_t best = 0;
    for (size_t i = 0; i < len;) {
        size_t step = text_utf8_len((unsigned char)s[i]);
        if (i + step > len)
            step = len - i;
        if (best > 0 && measure(s, i + step, ctx) > max_w)
            break;
        i += step;
        best = i;
    }
    return best;
}

int text_wrap(const char *s, int max_w, TextMeasureFn measure, void *ctx, TextLine *lines,
              int max_lines, bool *cut)
{
    size_t len = strlen(s);
    size_t pos = 0;
    int n = 0;
    *cut = false;

    while (n < max_lines) {
        while (pos < len && s[pos] == ' ')
            pos++;
        if (pos >= len)
            break;

        /* Extend the line word by word while it fits. */
        size_t end = pos, best = pos;
        bool forced = false;
        while (end < len && s[end] != '\n') {
            size_t word_end = end;
            while (word_end < len && s[word_end] == ' ')
                word_end++;
            while (word_end < len && s[word_end] != ' ' && s[word_end] != '\n')
                word_end++;
            if (measure(s + pos, word_end - pos, ctx) > max_w)
                break;
            best = end = word_end;
        }
        if (end < len && s[end] == '\n' && best == end)
            forced = true;
        if (best == pos) {
            /* A single word wider than the line: break inside it. */
            size_t word_end = pos;
            while (word_end < len && s[word_end] != ' ' && s[word_end] != '\n')
                word_end++;
            best = pos + fit_chars(s + pos, word_end - pos, max_w, measure, ctx);
        }

        lines[n].start = pos;
        lines[n].len = best - pos;
        n++;
        pos = forced ? best + 1 : best;
    }

    while (pos < len && (s[pos] == ' ' || s[pos] == '\n'))
        pos++;
    if (pos < len && n > 0) {
        *cut = true;
        lines[n - 1].len = len - lines[n - 1].start;
    }
    return n;
}
