#include "platform/transaction.h"

#include <stdlib.h>
#include <string.h>

#include "core/crc32.h"
#include "core/gamecoin.h"
#include "core/meet_save.h"
#include "core/panel_bits.h"
#include "platform/backup.h"
#include "platform/journal.h"

typedef enum {
    CHECK_FULL, /* sizes, unused bits, coin range */
    CHECK_SIZE, /* sizes only (restore) */
} CheckRule;

typedef struct {
    JournalOp op;
    BackupReason reason;
    CheckRule rule;
    /* Journal entry points here instead of the new backup (restore). */
    const char *journal_backup;
    const PurchasePlan *plan; /* purchase only, noted in the manifest */
} RunSpec;

typedef struct {
    uint8_t *meet_old;
    uint8_t *meet_new;
    uint8_t *scratch;
    uint8_t coin_old[GAMECOIN_SIZE];
    uint8_t coin_new[GAMECOIN_SIZE];
} Buffers;

static const char *const result_names[] = {
    "OK",          "NOTHING_TO_DO", "ERR_READ",     "ERR_INVALID", "ERR_REJECTED", "ERR_BACKUP",
    "ERR_JOURNAL", "ROLLED_BACK",   "ERR_ROLLBACK", "STOPPED",     "ERR_MEMORY",
};

const char *transaction_result_name(TxnResult result)
{
    if ((unsigned)result >= sizeof(result_names) / sizeof(result_names[0]))
        return "?";
    return result_names[result];
}

static bool data_ok(const uint8_t *meet, const uint8_t *coin, CheckRule rule)
{
    if (rule == CHECK_SIZE)
        return true;
    return meet_save_validate(meet, MEET_SAVE_SIZE) == MEET_OK &&
           gamecoin_validate(coin, GAMECOIN_SIZE) == COIN_OK;
}

/* Reads both game files; both must have their exact known size. */
static TxnResult read_game(const Storage *s, uint8_t *meet, uint8_t *coin)
{
    size_t meet_len = 0, coin_len = 0;
    StoreResult r = s->game_read(s->ctx, GAME_MEET, meet, MEET_SAVE_SIZE, &meet_len);
    if (r == STORE_ERR_SIZE)
        return TXN_ERR_INVALID;
    if (r != STORE_OK)
        return TXN_ERR_READ;
    r = s->game_read(s->ctx, GAME_COIN, coin, GAMECOIN_SIZE, &coin_len);
    if (r == STORE_ERR_SIZE)
        return TXN_ERR_INVALID;
    if (r != STORE_OK)
        return TXN_ERR_READ;
    if (meet_len != MEET_SAVE_SIZE || coin_len != GAMECOIN_SIZE)
        return TXN_ERR_INVALID;
    return TXN_OK;
}

/* Writes a game file and reads it back. */
static bool write_game(const Storage *s, GameFile file, const uint8_t *data, size_t len,
                       uint8_t *scratch)
{
    size_t got = 0;
    if (s->game_write(s->ctx, file, data, len) != STORE_OK)
        return false;
    if (s->game_read(s->ctx, file, scratch, len, &got) != STORE_OK || got != len)
        return false;
    return memcmp(scratch, data, len) == 0;
}

static void free_buffers(Buffers *b)
{
    free(b->meet_old);
    free(b->meet_new);
    free(b->scratch);
}

static TxnResult finish(TxnReport *rep, TxnResult result)
{
    rep->result = result;
    return result;
}

static TxnResult run(const TxnEnv *env, const RunSpec *spec, TxnChange change, void *arg,
                     TxnReport *rep)
{
    const Storage *s = env->storage;
    Buffers b;
    TxnResult r;

    memset(rep, 0, sizeof(*rep));
    memset(&b, 0, sizeof(b));
    b.meet_old = malloc(MEET_SAVE_SIZE);
    b.meet_new = malloc(MEET_SAVE_SIZE);
    b.scratch = malloc(MEET_SAVE_SIZE);
    if (b.meet_old == NULL || b.meet_new == NULL || b.scratch == NULL) {
        free_buffers(&b);
        return finish(rep, TXN_ERR_MEMORY);
    }

    /* 1: read fresh, never reuse what the caller showed on screen. */
    rep->step = TXN_STEP_READ;
    r = read_game(s, b.meet_old, b.coin_old);
    if (r != TXN_OK)
        goto out;

    /* 2: check, then apply the change to copies and check the result. */
    rep->step = TXN_STEP_CHECK;
    if (!data_ok(b.meet_old, b.coin_old, spec->rule)) {
        r = TXN_ERR_INVALID;
        goto out;
    }
    memcpy(b.meet_new, b.meet_old, MEET_SAVE_SIZE);
    memcpy(b.coin_new, b.coin_old, GAMECOIN_SIZE);
    rep->reject = change(b.meet_new, MEET_SAVE_SIZE, b.coin_new, GAMECOIN_SIZE, arg);
    if (rep->reject != 0) {
        r = TXN_ERR_REJECTED;
        goto out;
    }
    if (!data_ok(b.meet_new, b.coin_new, spec->rule)) {
        r = TXN_ERR_INVALID;
        goto out;
    }
    bool meet_changed = memcmp(b.meet_new, b.meet_old, MEET_SAVE_SIZE) != 0;
    bool coin_changed = memcmp(b.coin_new, b.coin_old, GAMECOIN_SIZE) != 0;
    if (!meet_changed && !coin_changed) {
        r = TXN_NOTHING_TO_DO;
        goto out;
    }

    /* 3: backup of the state before the change. */
    rep->step = TXN_STEP_BACKUP;
    Manifest meta;
    memset(&meta, 0, sizeof(meta));
    strncpy(meta.app_version, env->app_version, sizeof(meta.app_version) - 1);
    meta.created = env->now;
    meta.reason = spec->reason;
    meta.title_id = env->title_id;
    meta.has_coins = gamecoin_get(b.coin_old, GAMECOIN_SIZE, &meta.coins) == COIN_OK;
    if (spec->plan != NULL) {
        meta.has_purchase = true;
        meta.panel = spec->plan->panel_id;
        meta.pieces = (uint8_t)panel_bits_count(spec->plan->pieces);
    }
    if (backup_choose_name(s, &env->now, rep->backup) != BACKUP_OK ||
        backup_create(s, rep->backup, &meta, b.meet_old, MEET_SAVE_SIZE, b.coin_old, GAMECOIN_SIZE,
                      NULL) != BACKUP_OK) {
        rep->backup[0] = '\0';
        r = TXN_ERR_BACKUP;
        goto out;
    }
    if (env->stop_after == TXN_STEP_BACKUP) {
        r = TXN_STOPPED;
        goto out;
    }

    /* 4: journal. From here on an interruption shows E-06 at the next start. */
    rep->step = TXN_STEP_JOURNAL;
    JournalRecord rec;
    memset(&rec, 0, sizeof(rec));
    rec.op = spec->op;
    strcpy(rec.backup, spec->journal_backup ? spec->journal_backup : rep->backup);
    rec.meet_crc_before = crc32(b.meet_old, MEET_SAVE_SIZE);
    rec.coin_crc_before = crc32(b.coin_old, GAMECOIN_SIZE);
    rec.meet_crc_after = crc32(b.meet_new, MEET_SAVE_SIZE);
    rec.coin_crc_after = crc32(b.coin_new, GAMECOIN_SIZE);
    if (journal_write(s, &rec) != STORE_OK) {
        journal_clear(s);
        r = TXN_ERR_JOURNAL;
        goto out;
    }
    if (env->stop_after == TXN_STEP_JOURNAL) {
        r = TXN_STOPPED;
        goto out;
    }

    /* 5: pieces first. */
    rep->step = TXN_STEP_MEET;
    if (meet_changed) {
        rep->meet_written = true;
        if (!write_game(s, GAME_MEET, b.meet_new, MEET_SAVE_SIZE, b.scratch)) {
            bool back = write_game(s, GAME_MEET, b.meet_old, MEET_SAVE_SIZE, b.scratch);
            r = back ? TXN_ROLLED_BACK : TXN_ERR_ROLLBACK;
            goto rolled;
        }
    }
    if (env->stop_after == TXN_STEP_MEET) {
        r = TXN_STOPPED;
        goto out;
    }

    /* 6: coins. On failure both files go back. */
    rep->step = TXN_STEP_COIN;
    if (coin_changed) {
        rep->coin_written = true;
        if (!write_game(s, GAME_COIN, b.coin_new, GAMECOIN_SIZE, b.scratch)) {
            bool back = write_game(s, GAME_COIN, b.coin_old, GAMECOIN_SIZE, b.scratch);
            if (meet_changed)
                back = write_game(s, GAME_MEET, b.meet_old, MEET_SAVE_SIZE, b.scratch) && back;
            r = back ? TXN_ROLLED_BACK : TXN_ERR_ROLLBACK;
            goto rolled;
        }
    }
    if (env->stop_after == TXN_STEP_COIN) {
        r = TXN_STOPPED;
        goto out;
    }

    /* 7: done. Old backups only go after a purchase, never the one just restored. */
    rep->step = TXN_STEP_FINISH;
    if (journal_clear(s) != STORE_OK)
        rep->cleanup_failed = true;
    if (spec->op == JOURNAL_OP_PURCHASE)
        backup_prune(s, env->keep_backups ? env->keep_backups : BACKUP_KEEP_DEFAULT);
    r = TXN_OK;
    goto out;

rolled:
    /* Old data is back: the journal would only cause a needless E-06. */
    if (r == TXN_ROLLED_BACK && journal_clear(s) != STORE_OK)
        rep->cleanup_failed = true;
out:
    free_buffers(&b);
    return finish(rep, r);
}

static int apply_plan(uint8_t *meet, size_t meet_len, uint8_t *coin, size_t coin_len, void *arg)
{
    ShopResult r = shop_plan_apply(meet, meet_len, coin, coin_len, arg);
    return r == SHOP_OK ? 0 : (int)r;
}

TxnResult transaction_purchase(const TxnEnv *env, const PurchasePlan *plan, TxnReport *rep)
{
    RunSpec spec = {JOURNAL_OP_PURCHASE, BACKUP_REASON_PURCHASE, CHECK_FULL, NULL, plan};
    return run(env, &spec, apply_plan, (void *)plan, rep);
}

typedef struct {
    const Storage *storage;
    const char *name;
} RestoreArg;

static int load_backup(uint8_t *meet, size_t meet_len, uint8_t *coin, size_t coin_len, void *arg)
{
    const RestoreArg *ra = arg;
    Manifest m;
    size_t got_meet, got_coin;
    BackupResult r = backup_load(ra->storage, ra->name, &m, meet, meet_len, &got_meet, coin,
                                 coin_len, &got_coin);
    if (r != BACKUP_OK)
        return (int)r;
    /* A backup with other sizes cannot come from this game data. */
    if (got_meet != meet_len || got_coin != coin_len)
        return (int)BACKUP_ERR_DAMAGED;
    return 0;
}

TxnResult transaction_restore(const TxnEnv *env, const char *backup_name, TxnReport *rep)
{
    if (!backup_name_valid(backup_name)) {
        memset(rep, 0, sizeof(*rep));
        rep->reject = (int)BACKUP_ERR_NAME;
        return finish(rep, TXN_ERR_REJECTED);
    }
    RunSpec spec = {JOURNAL_OP_RESTORE, BACKUP_REASON_RESTORE, CHECK_SIZE, backup_name, NULL};
    RestoreArg arg = {env->storage, backup_name};
    return run(env, &spec, load_backup, &arg, rep);
}

TxnResult transaction_change(const TxnEnv *env, TxnChange change, void *arg, TxnReport *rep)
{
    RunSpec spec = {JOURNAL_OP_DIAG, BACKUP_REASON_DIAG, CHECK_FULL, NULL, NULL};
    return run(env, &spec, change, arg, rep);
}

/* Current CRCs of both game files. */
static TxnResult current_crcs(const Storage *s, uint32_t *meet_crc, uint32_t *coin_crc)
{
    uint8_t *meet = malloc(MEET_SAVE_SIZE);
    uint8_t coin[GAMECOIN_SIZE];
    if (meet == NULL)
        return TXN_ERR_MEMORY;
    TxnResult r = read_game(s, meet, coin);
    if (r == TXN_OK) {
        *meet_crc = crc32(meet, MEET_SAVE_SIZE);
        *coin_crc = crc32(coin, GAMECOIN_SIZE);
    }
    free(meet);
    return r;
}

RecoveryStatus transaction_check_recovery(const TxnEnv *env, RecoveryInfo *info)
{
    const Storage *s = env->storage;
    uint32_t meet_crc = 0, coin_crc = 0;
    memset(info, 0, sizeof(*info));

    JournalStatus js = journal_read(s, &info->rec);
    if (js == JOURNAL_NONE)
        return RECOVERY_NONE;
    if (js == JOURNAL_IO)
        return RECOVERY_ERR_IO;

    TxnResult r = current_crcs(s, &meet_crc, &coin_crc);
    if (r == TXN_ERR_READ || r == TXN_ERR_MEMORY)
        return RECOVERY_ERR_IO;

    if (js == JOURNAL_DAMAGED) {
        /*
         * The journal is written once, before any game write, so a damaged one
         * means the interruption came while writing it. If the game data still
         * equals the newest backup, nothing was changed.
         */
        BackupInfo newest;
        memset(&info->rec, 0, sizeof(info->rec));
        if (r == TXN_OK && backup_list(s, &newest, 1) > 0 && newest.manifest.meet_crc == meet_crc &&
            newest.manifest.coin_crc == coin_crc) {
            journal_clear(s);
            return RECOVERY_CLEANED;
        }
        return RECOVERY_DAMAGED;
    }

    if (r == TXN_OK) {
        info->meet_is_before = meet_crc == info->rec.meet_crc_before;
        info->meet_is_after = meet_crc == info->rec.meet_crc_after;
        info->coin_is_before = coin_crc == info->rec.coin_crc_before;
        info->coin_is_after = coin_crc == info->rec.coin_crc_after;
    }

    /* Untouched or fully done: the journal is only a leftover. */
    bool untouched = info->meet_is_before && info->coin_is_before;
    bool complete = info->meet_is_after && info->coin_is_after;
    if (untouched || complete) {
        journal_clear(s);
        return RECOVERY_CLEANED;
    }
    return RECOVERY_INTERRUPTED;
}
