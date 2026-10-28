#ifndef PSS_PLATFORM_FAULT_STORAGE_H
#define PSS_PLATFORM_FAULT_STORAGE_H

#include <stdbool.h>

#include "platform/storage.h"

/*
 * Diagnosis build only: wraps a Storage and makes one step of a transaction
 * fail, so that every error path can be tried on the console.
 *
 * A failing call either does nothing ("before") or does the real work and
 * then lies about it ("after", the read-back sees a changed byte). The second
 * kind makes the transaction write the old data back for real.
 */

typedef enum {
    FAULT_NONE = 0,
    FAULT_BACKUP_WRITE,  /* first file of the backup cannot be written: E-04 */
    FAULT_JOURNAL_WRITE, /* journal cannot be written: E-04 */
    FAULT_MEET_WRITE,    /* meet.dat write fails, nothing written: E-04 */
    FAULT_MEET_VERIFY,   /* meet.dat written, read-back differs: rolled back, E-04 */
    FAULT_COIN_WRITE,    /* gamecoin.dat write fails: meet.dat rolled back, E-04 */
    FAULT_COIN_VERIFY,   /* gamecoin.dat written, read-back differs: both rolled back, E-04 */
    FAULT_ROLLBACK,      /* every meet.dat write fails, so does going back: E-05 */
    FAULT_COUNT,
} FaultScenario;

typedef struct {
    const Storage *inner;
    FaultScenario scenario;
    unsigned fired;    /* how often the fault was injected */
    bool corrupt_meet; /* next meet.dat read returns a changed byte */
    bool corrupt_coin;
    Storage storage; /* hand this to the transaction */
} FaultStorage;

void fault_storage_init(FaultStorage *f, const Storage *inner, FaultScenario scenario);

/* Short German label for the diagnosis menu. */
const char *fault_scenario_name(FaultScenario scenario);

#endif
