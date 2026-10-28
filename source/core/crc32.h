#ifndef PSS_CORE_CRC32_H
#define PSS_CORE_CRC32_H

#include <stddef.h>
#include <stdint.h>

/* CRC-32 (IEEE 802.3, reflected, polynomial 0xEDB88320). */
uint32_t crc32_update(uint32_t crc, const void *data, size_t len);

static inline uint32_t crc32(const void *data, size_t len)
{
    return crc32_update(0, data, len);
}

#endif
