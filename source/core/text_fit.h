#ifndef PSS_CORE_TEXT_FIT_H
#define PSS_CORE_TEXT_FIT_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Shortening of text that does not fit a fixed width. The caller supplies the width
 * of a byte range so this stays independent of the font.
 */

#define TEXT_FIT_ELLIPSIS "\xE2\x80\xA6" /* U+2026 */

/* Width in pixels of the first len bytes of s. Must grow with len. */
typedef int (*TextMeasureFn)(const char *s, size_t len, void *ctx);

/*
 * Returns how many bytes of the UTF-8 string s to draw so that they fit
 * max_w. If the whole string fits, that is strlen(s) and *cut is false.
 * Otherwise it is the longest prefix, ending on a character boundary and
 * without trailing spaces, whose width plus ellipsis_w fits; *cut is true
 * and the caller appends TEXT_FIT_ELLIPSIS. The result may be 0.
 */
size_t text_fit(const char *s, int max_w, int ellipsis_w, TextMeasureFn measure, void *ctx,
                bool *cut);

typedef struct {
    size_t start; /* byte offset into the text */
    size_t len;   /* bytes, without the separating space or newline */
} TextLine;

/*
 * Breaks s into lines no wider than max_w: at spaces, at '\n', and inside a
 * word only if the word alone is too wide. Leading spaces of a line are
 * skipped. Writes at most max_lines lines and returns their number.
 * If the text needs more lines, *cut is true and the last line runs to the
 * end of the text; draw that one shortened with an ellipsis (text_fit).
 */
int text_wrap(const char *s, int max_w, TextMeasureFn measure, void *ctx, TextLine *lines,
              int max_lines, bool *cut);

/* Length of the UTF-8 sequence starting with byte c (1 for invalid lead bytes). */
size_t text_utf8_len(unsigned char c);

#endif
