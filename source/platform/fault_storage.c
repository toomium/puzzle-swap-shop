#include "platform/fault_storage.h"

#include <string.h>

#include "platform/journal.h"

static const char *const names[FAULT_COUNT] = {
    "kein Fehler",
    "Sicherung schreiben scheitert",
    "Journal schreiben scheitert",
    "meet.dat schreiben scheitert",
    "meet.dat falsch zurueckgelesen",
    "gamecoin.dat schreiben scheitert",
    "gamecoin.dat falsch zurueckgelesen",
    "Rueckbau scheitert",
};

const char *fault_scenario_name(FaultScenario scenario)
{
    return (unsigned)scenario < FAULT_COUNT ? names[scenario] : "?";
}

static StoreResult game_read(void *ctx, GameFile file, uint8_t *buf, size_t cap, size_t *len)
{
    FaultStorage *f = ctx;
    StoreResult r = f->inner->game_read(f->inner->ctx, file, buf, cap, len);
    bool *corrupt = file == GAME_MEET ? &f->corrupt_meet : &f->corrupt_coin;
    if (r == STORE_OK && *corrupt && *len > 0) {
        *corrupt = false;
        buf[*len - 1] ^= 0x01;
    }
    return r;
}

static StoreResult game_write(void *ctx, GameFile file, const uint8_t *buf, size_t len)
{
    FaultStorage *f = ctx;
    bool once = f->fired == 0;
    switch (f->scenario) {
    case FAULT_MEET_WRITE:
        if (file == GAME_MEET && once) {
            f->fired++;
            return STORE_ERR_IO;
        }
        break;
    case FAULT_COIN_WRITE:
        if (file == GAME_COIN && once) {
            f->fired++;
            return STORE_ERR_IO;
        }
        break;
    case FAULT_ROLLBACK:
        if (file == GAME_MEET) {
            f->fired++;
            return STORE_ERR_IO;
        }
        break;
    default:
        break;
    }
    StoreResult r = f->inner->game_write(f->inner->ctx, file, buf, len);
    if (r == STORE_OK && once) {
        if (f->scenario == FAULT_MEET_VERIFY && file == GAME_MEET) {
            f->fired++;
            f->corrupt_meet = true;
        } else if (f->scenario == FAULT_COIN_VERIFY && file == GAME_COIN) {
            f->fired++;
            f->corrupt_coin = true;
        }
    }
    return r;
}

static StoreResult sd_read(void *ctx, const char *path, uint8_t *buf, size_t cap, size_t *len)
{
    FaultStorage *f = ctx;
    return f->inner->sd_read(f->inner->ctx, path, buf, cap, len);
}

static StoreResult sd_write(void *ctx, const char *path, const uint8_t *buf, size_t len)
{
    FaultStorage *f = ctx;
    if (f->fired == 0) {
        bool backup = strncmp(path, "backups/", 8) == 0;
        bool journal = strcmp(path, JOURNAL_FILE) == 0;
        if ((f->scenario == FAULT_BACKUP_WRITE && backup) ||
            (f->scenario == FAULT_JOURNAL_WRITE && journal)) {
            f->fired++;
            return STORE_ERR_IO;
        }
    }
    return f->inner->sd_write(f->inner->ctx, path, buf, len);
}

static StoreResult sd_remove(void *ctx, const char *path)
{
    FaultStorage *f = ctx;
    return f->inner->sd_remove(f->inner->ctx, path);
}

static StoreResult sd_mkdir(void *ctx, const char *path)
{
    FaultStorage *f = ctx;
    return f->inner->sd_mkdir(f->inner->ctx, path);
}

static StoreResult sd_rmdir(void *ctx, const char *path)
{
    FaultStorage *f = ctx;
    return f->inner->sd_rmdir(f->inner->ctx, path);
}

static StoreResult sd_list(void *ctx, const char *path, StoreListFn fn, void *arg)
{
    FaultStorage *f = ctx;
    return f->inner->sd_list(f->inner->ctx, path, fn, arg);
}

void fault_storage_init(FaultStorage *f, const Storage *inner, FaultScenario scenario)
{
    memset(f, 0, sizeof(*f));
    f->inner = inner;
    f->scenario = scenario;
    f->storage = (Storage){f,        game_read, game_write, sd_read, sd_write, sd_remove,
                           sd_mkdir, sd_rmdir,  sd_list,    NULL,    NULL};
}
