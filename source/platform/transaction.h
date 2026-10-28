#ifndef PSS_PLATFORM_TRANSACTION_H
#define PSS_PLATFORM_TRANSACTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/backup_names.h"
#include "core/journal_record.h"
#include "core/manifest.h"
#include "core/shop.h"
#include "platform/storage.h"

/*
 * The only code that writes game data. Follows the steps
 * in this order:
 *
 *   1 read both files fresh   2 check, apply the change in memory
 *   3 backup and verify       4 journal, written once
 *   5 meet.dat, verify        6 gamecoin.dat, verify
 *   7 remove journal
 *
 * Pieces are written before coins, so an interruption between 5 and 6 never
 * costs coins. The caller keeps HOME disabled while a transaction runs.
 */

typedef enum {
    TXN_STEP_NONE = 0,
    TXN_STEP_READ,
    TXN_STEP_CHECK,
    TXN_STEP_BACKUP,
    TXN_STEP_JOURNAL,
    TXN_STEP_MEET,
    TXN_STEP_COIN,
    TXN_STEP_FINISH,
} TxnStep;

typedef struct {
    const Storage *storage;
    uint64_t title_id;
    Timestamp now; /* names the backup */
    const char *app_version;
    unsigned keep_backups; /* after a purchase; 0 = BACKUP_KEEP_DEFAULT */
    /* Diagnostic build only: stop after this step as if the power failed. */
    TxnStep stop_after;
} TxnEnv;

typedef enum {
    TXN_OK = 0,
    TXN_NOTHING_TO_DO, /* the change left both files as they were; nothing written */
    TXN_ERR_READ,      /* game data unreadable; nothing changed */
    TXN_ERR_INVALID,   /* game data failed the checks; nothing changed (E-03) */
    TXN_ERR_REJECTED,  /* the change refused, see TxnReport.reject; nothing changed */
    TXN_ERR_BACKUP,    /* backup failed; nothing changed (E-04) */
    TXN_ERR_JOURNAL,   /* journal failed; nothing changed (E-04) */
    TXN_ROLLED_BACK,   /* writing failed, the old data is back (E-04) */
    TXN_ERR_ROLLBACK,  /* writing failed and so did going back; journal kept (E-05) */
    TXN_STOPPED,       /* stopped on purpose by stop_after; journal kept */
    TXN_ERR_MEMORY,
} TxnResult;

typedef struct {
    TxnResult result;
    TxnStep step;                 /* step reached, or the one that failed */
    int reject;                   /* return value of the change when refused */
    char backup[BACKUP_NAME_MAX]; /* backup made before the change */
    bool meet_written;
    bool coin_written;
    bool cleanup_failed; /* journal not removed or pruning failed; harmless */
} TxnReport;

/*
 * Changes the copies of both files in place. Returns 0 to go ahead and any
 * other value to refuse; that value ends up in TxnReport.reject.
 */
typedef int (*TxnChange)(uint8_t *meet, size_t meet_len, uint8_t *coin, size_t coin_len, void *arg);

/* Buys plan->pieces for plan->cost coins. reject is a ShopResult. */
TxnResult transaction_purchase(const TxnEnv *env, const PurchasePlan *plan, TxnReport *rep);

/*
 * Writes a backup back onto the game data, after backing up the current state
 * (reason "restore"). The current data does not have to pass the checks, so a
 * damaged save can be repaired. reject is a BackupResult.
 */
TxnResult transaction_restore(const TxnEnv *env, const char *backup_name, TxnReport *rep);

/* Any other change, for the diagnostic build. Both results must pass the checks. */
TxnResult transaction_change(const TxnEnv *env, TxnChange change, void *arg, TxnReport *rep);

typedef enum {
    RECOVERY_NONE = 0,    /* no journal */
    RECOVERY_CLEANED,     /* journal left over, but the data is consistent; removed */
    RECOVERY_INTERRUPTED, /* a change stopped half way (E-06); info->rec.backup restores */
    RECOVERY_DAMAGED,     /* journal unreadable; let the user pick a backup */
    RECOVERY_ERR_IO,      /* SD card or game data unreadable */
} RecoveryStatus;

typedef struct {
    JournalRecord rec;
    bool meet_is_before; /* current meet.dat matches the state before the change */
    bool meet_is_after;
    bool coin_is_before;
    bool coin_is_after;
} RecoveryInfo;

/* Run at start-up, before anything else touches the game data. */
RecoveryStatus transaction_check_recovery(const TxnEnv *env, RecoveryInfo *info);

const char *transaction_result_name(TxnResult result);

#endif
