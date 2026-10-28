#ifndef PSS_PLATFORM_CLOCK_H
#define PSS_PLATFORM_CLOCK_H

#include <stdbool.h>

#include "core/backup_names.h"

/* Current time of the system clock; false if it is not set to 2000..9999. */
bool clock_now(Timestamp *out);

#endif
