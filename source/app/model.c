#include "app/model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/lz11.h"
#include "core/meet_save.h"
#include "core/panel_bits.h"
#include "core/rare_pieces.h"
#include "platform/journal.h"

static void make_env(const Model *m, TxnEnv *env)
{
    memset(env, 0, sizeof(*env));
    env->storage = m->env.storage;
    env->title_id = m->env.title->title_id;
    if (m->env.now == NULL || !m->env.now(&env->now))
        env->now = (Timestamp){2000, 1, 1, 0, 0, 0};
    env->app_version = m->env.app_version;
}

static void load_settings(Model *m)
{
    char text[SETTINGS_TEXT_MAX];
    size_t len = 0;
    const Storage *s = m->env.storage;
    m->settings = settings_default();
    m->settings_bad = 0;
    StoreResult r = s->sd_read(s->ctx, SETTINGS_FILE, (uint8_t *)text, sizeof(text), &len);
    if (r == STORE_OK && !settings_parse(text, len, &m->settings, &m->settings_bad))
        m->settings_bad++;
    else if (r != STORE_OK && r != STORE_ERR_NOT_FOUND)
        m->settings_bad++;
    /* First start: write the defaults, so the file can be found and edited. */
    if (r == STORE_ERR_NOT_FOUND)
        model_save_settings(m, &m->settings);
}

bool model_panel_present(unsigned id, uint64_t owned)
{
    const PanelInfo *info = panel_catalog_get(id);
    if (info == NULL)
        return false;
    /*
     * Without PNEL data the app cannot tell whether a SpotPass panel is
     * downloaded, so only panels with at least one piece count as present and
     * the shop never sells pieces for a panel the console may not have.
     */
    return info->kind == PANEL_STANDARD || owned != 0;
}

static void build_panels(Model *m)
{
    for (unsigned id = 0; id < PANEL_COUNT; id++) {
        const PanelInfo *info = panel_catalog_get(id);
        PanelState *p = &m->panels[id];
        memset(p, 0, sizeof(*p));
        p->id = (uint8_t)id;
        p->piece_count = info->piece_count;
        if (m->has_data)
            meet_save_get_owned(m->meet, MEET_SAVE_SIZE, id, &p->owned);
        const PnelPanel *listed = &m->pnel.index.panel[id];
        p->available = pnel_index_panel_present(&m->pnel.index, id, p->owned);
        const char *name = listed->name[0] != '\0' ? listed->name : info->fallback_name;
        strncpy(p->name, name, sizeof(p->name) - 1);
        if (listed->present)
            rare_mask_from_pnel(listed->pink, info->piece_count, &p->rare);
    }

    /* Incomplete first, then complete, then missing panels; each by number. */
    unsigned n = 0;
    for (int group = 0; group < 3; group++) {
        for (unsigned id = 0; id < PANEL_COUNT; id++) {
            const PanelState *p = &m->panels[id];
            bool complete = p->owned == panel_bits_all(p->piece_count);
            int g = !p->available ? 2 : complete ? 1 : 0;
            if (g == group)
                m->gallery[n++] = (uint8_t)id;
        }
    }
}

/* Reads the game data; returns false if it could not be read at all. */
static bool read_game(Model *m)
{
    const Storage *s = m->env.storage;
    size_t ml = 0, cl = 0;
    m->has_data = false;
    m->coins = 0;
    StoreResult a = s->game_read(s->ctx, GAME_MEET, m->meet, MEET_SAVE_SIZE, &ml);
    StoreResult b = s->game_read(s->ctx, GAME_COIN, m->coin, GAMECOIN_SIZE, &cl);
    if ((a != STORE_OK && a != STORE_ERR_SIZE) || (b != STORE_OK && b != STORE_ERR_SIZE))
        return false;
    m->has_data = a == STORE_OK && b == STORE_OK && ml == MEET_SAVE_SIZE && cl == GAMECOIN_SIZE;
    if (m->has_data)
        gamecoin_get(m->coin, GAMECOIN_SIZE, &m->coins);
    return true;
}

static ModelStatus decide(Model *m, bool readable)
{
    if (!readable)
        return MODEL_ERR_READ;
    if (m->recovery == RECOVERY_INTERRUPTED || m->recovery == RECOVERY_DAMAGED)
        return MODEL_INTERRUPTED;
    if (!m->has_data || meet_save_validate(m->meet, MEET_SAVE_SIZE) != MEET_OK ||
        gamecoin_validate(m->coin, GAMECOIN_SIZE) != COIN_OK)
        return MODEL_READ_ONLY;
    if (!backup_exists(m->env.storage, BACKUP_ORIGINAL))
        return MODEL_NEED_ORIGINAL;
    return MODEL_READY;
}

ModelStatus model_refresh(Model *m)
{
    bool readable = read_game(m);
    build_panels(m);
    m->status = decide(m, readable);
    return m->status;
}

ModelStatus model_load(Model *m, const ModelEnv *env)
{
    memset(m, 0, sizeof(*m));
    m->env = *env;
    m->meet = malloc(MEET_SAVE_SIZE);
    if (m->meet == NULL) {
        m->status = MODEL_ERR_READ;
        return m->status;
    }
    load_settings(m);
    m->pnel_status = pnel_load(env->storage, env->system_language, &m->pnel);

    TxnEnv tenv;
    make_env(m, &tenv);
    m->recovery = transaction_check_recovery(&tenv, &m->recovery_info);
    if (m->recovery == RECOVERY_ERR_IO) {
        m->status = MODEL_ERR_READ;
        return m->status;
    }
    return model_refresh(m);
}

void model_free(Model *m)
{
    free(m->meet);
    m->meet = NULL;
}

BackupResult model_create_original(Model *m)
{
    if (!m->has_data)
        return BACKUP_ERR_IO;
    TxnEnv env;
    Manifest meta;
    make_env(m, &env);
    memset(&meta, 0, sizeof(meta));
    strncpy(meta.app_version, env.app_version, sizeof(meta.app_version) - 1);
    meta.created = env.now;
    meta.reason = BACKUP_REASON_ORIGINAL;
    meta.title_id = env.title_id;
    meta.has_coins = true;
    meta.coins = m->coins;
    BackupResult r = backup_create(m->env.storage, BACKUP_ORIGINAL, &meta, m->meet, MEET_SAVE_SIZE,
                                   m->coin, GAMECOIN_SIZE, NULL);
    model_refresh(m);
    return r;
}

/* Copies a picture block into buf, unpacking it when it is LZ11 packed. */
static StoreResult unpack_image(const uint8_t *raw, size_t raw_len, uint8_t *buf, size_t cap,
                                size_t *len)
{
    if (lz11_is_packed(raw, raw_len)) {
        *len = lz11_unpack(raw, raw_len, buf, cap);
        if (*len == 0)
            return lz11_unpacked_size(raw, raw_len) > cap ? STORE_ERR_SIZE : STORE_ERR_IO;
        return STORE_OK;
    }
    if (raw_len > cap)
        return STORE_ERR_SIZE;
    memcpy(buf, raw, raw_len);
    *len = raw_len;
    return STORE_OK;
}

#define BUILTIN_IMAGE_MAX (256u * 1024)

bool model_panel_has_image(const Model *m, unsigned id)
{
    return id < PANEL_COUNT &&
           (m->pnel.index.panel[id].present || panel_catalog_builtin_image(id) != NULL);
}

unsigned model_builtin_pictures_missing(const Model *m)
{
    unsigned missing = 0;
    for (unsigned id = 0; id < PANEL_COUNT; id++) {
        const char *file = panel_catalog_builtin_image(id);
        if (file == NULL || m->pnel.index.panel[id].present)
            continue;
        char path[64];
        uint8_t probe[4];
        size_t len = 0;
        snprintf(path, sizeof(path), "panels/%s", file);
        /* STORE_ERR_SIZE only says that the file exists and is bigger than the probe. */
        if (m->env.storage->sd_read(m->env.storage->ctx, path, probe, sizeof(probe), &len) ==
            STORE_ERR_NOT_FOUND)
            missing++;
    }
    return missing;
}

StoreResult model_panel_image(const Model *m, unsigned id, uint8_t *buf, size_t cap, size_t *len)
{
    *len = 0;
    if (id >= PANEL_COUNT)
        return STORE_ERR_NOT_FOUND;
    if (!m->pnel.index.panel[id].present) {
        /* Built-in panels have no PNEL entry; their pictures were copied from the RomFS of
         * Mii Plaza to the SD card. */
        const char *file = panel_catalog_builtin_image(id);
        if (file == NULL)
            return STORE_ERR_NOT_FOUND;
        char path[64];
        snprintf(path, sizeof(path), "panels/%s", file);
        uint8_t *raw = malloc(BUILTIN_IMAGE_MAX);
        if (raw == NULL)
            return STORE_ERR_IO;
        size_t raw_len = 0;
        StoreResult r =
            m->env.storage->sd_read(m->env.storage->ctx, path, raw, BUILTIN_IMAGE_MAX, &raw_len);
        if (r == STORE_OK)
            r = unpack_image(raw, raw_len, buf, cap, len);
        free(raw);
        return r;
    }
    size_t raw_len = m->pnel.index.panel[id].image_size;
    uint8_t *raw = malloc(raw_len ? raw_len : 1);
    if (raw == NULL)
        return STORE_ERR_IO;
    StoreResult r = pnel_read_image(m->env.storage, &m->pnel, id, raw, raw_len, &raw_len);
    if (r == STORE_OK)
        r = unpack_image(raw, raw_len, buf, cap, len);
    free(raw);
    return r;
}

const PanelState *model_panel(const Model *m, unsigned id)
{
    return id < PANEL_COUNT ? &m->panels[id] : NULL;
}

uint64_t model_buyable(const Model *m, unsigned id)
{
    const PanelState *p = model_panel(m, id);
    return p != NULL ? shop_buyable(p, &m->settings.prices) : 0;
}

ShopResult model_plan(const Model *m, unsigned panel, uint64_t pieces, PurchasePlan *out)
{
    const PanelState *p = model_panel(m, panel);
    if (p == NULL)
        return SHOP_ERR_UNAVAILABLE;
    if (m->status != MODEL_READY && m->status != MODEL_NEED_ORIGINAL)
        return SHOP_ERR_DATA;
    return shop_plan_build(p, pieces, m->coins, &m->settings.prices, out);
}

static TxnResult refuse(Model *m)
{
    memset(&m->last, 0, sizeof(m->last));
    m->last.result = TXN_ERR_REJECTED;
    m->last.reject = (int)SHOP_ERR_DATA;
    return m->last.result;
}

/* After any write: read again; a failed rollback must be resolved first. */
static TxnResult after_write(Model *m, TxnResult r)
{
    if (r == TXN_ERR_ROLLBACK) {
        TxnEnv env;
        make_env(m, &env);
        m->recovery = transaction_check_recovery(&env, &m->recovery_info);
    } else if (r == TXN_OK) {
        m->recovery = RECOVERY_NONE;
        memset(&m->recovery_info, 0, sizeof(m->recovery_info));
    }
    model_refresh(m);
    return r;
}

TxnResult model_buy(Model *m, const PurchasePlan *plan)
{
    TxnEnv env;
    if (m->status != MODEL_READY)
        return refuse(m);
    make_env(m, &env);
    return after_write(m, transaction_purchase(&env, plan, &m->last));
}

TxnResult model_restore(Model *m, const char *backup)
{
    TxnEnv env;
    if (m->status == MODEL_ERR_READ)
        return refuse(m);
    make_env(m, &env);
    return after_write(m, transaction_restore(&env, backup, &m->last));
}

TxnResult model_resolve_interrupted(Model *m, bool restore)
{
    if (m->status != MODEL_INTERRUPTED)
        return refuse(m);
    if (restore) {
        if (m->recovery != RECOVERY_INTERRUPTED)
            return refuse(m);
        return model_restore(m, m->recovery_info.rec.backup);
    }
    memset(&m->last, 0, sizeof(m->last));
    if (journal_clear(m->env.storage) != STORE_OK) {
        m->last.result = TXN_ERR_JOURNAL;
        return m->last.result;
    }
    m->recovery = RECOVERY_NONE;
    memset(&m->recovery_info, 0, sizeof(m->recovery_info));
    model_refresh(m);
    m->last.result = TXN_OK;
    return TXN_OK;
}

unsigned model_backups(const Model *m, BackupInfo *out, unsigned max)
{
    return backup_list(m->env.storage, out, max);
}

bool model_save_settings(Model *m, const Settings *s)
{
    char text[SETTINGS_TEXT_MAX];
    size_t len;
    const Storage *st = m->env.storage;
    if (!settings_format(s, text, sizeof(text), &len) ||
        st->sd_write(st->ctx, SETTINGS_FILE, (const uint8_t *)text, len) != STORE_OK)
        return false;
    m->settings = *s;
    m->settings_bad = 0;
    return true;
}
