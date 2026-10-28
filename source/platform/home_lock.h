#ifndef PSS_PLATFORM_HOME_LOCK_H
#define PSS_PLATFORM_HOME_LOCK_H

#include <stdbool.h>

/*
 * Keeps the HOME button and sleep mode from interrupting a purchase. Lock
 * before model_buy or model_restore, unlock afterwards.
 */
void home_lock(bool locked);

#endif
