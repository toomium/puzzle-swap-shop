#include "platform/home_lock.h"

#include <3ds.h>

void home_lock(bool locked)
{
    aptSetHomeAllowed(!locked);
    aptSetSleepAllowed(!locked);
}
