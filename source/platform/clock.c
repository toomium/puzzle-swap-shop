#include "platform/clock.h"

#include <time.h>

bool clock_now(Timestamp *out)
{
    /* libctru's time() already counts in the console's local time. */
    return timestamp_from_unix((int64_t)time(NULL), out);
}
