#ifndef PSS_CORE_SIZE_TEXT_H
#define PSS_CORE_SIZE_TEXT_H

#include <stddef.h>
#include <stdint.h>

/* File sizes for the screens: "20 B", "384 KB", "2 MB". Rounded up to whole units. */
int size_text_format(char *out, size_t size, uint32_t bytes);

#endif
