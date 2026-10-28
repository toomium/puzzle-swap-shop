#ifndef PSS_APP_MODEL_H
#define PSS_APP_MODEL_H

#include <stdbool.h>
#include <stdint.h>

#include "core/gamecoin.h"
#include "core/mii_plaza.h"
#include "core/panel_catalog.h"
#include "app/pnel_load.h"
#include "core/panel_state.h"
#include "core/settings.h"
#include "core/shop.h"
#include "platform/backup.h"
#include "platform/storage.h"
#include "platform/transaction.h"

/*
 * State of the app between the screens and the platform. Screens read the fields below and call the
 * model_* commands; they never touch files. The model itself only talks to ModelEnv, so the host
 * tests run it on the in-memory storage; app/boot.c sets it up on the console.
 */

typedef enum {
    MODEL_READY = 0,     /* S2: purchases allowed */
    MODEL_NEED_ORIGINAL, /* S1: no original backup yet; purchases blocked */
    MODEL_READ_ONLY,     /* E-03: save data looks unknown; view only */
    MODEL_INTERRUPTED,   /* E-06: an interrupted change needs a decision */
    MODEL_ERR_READ,      /* E-01: game data or app folder cannot be read */
} ModelStatus;

typedef struct {
    const Storage *storage;
    const PlazaTitle *title;
    bool (*now)(Timestamp *out); /* names backups; false falls back to 2000-01-01 */
    const char *app_version;
    unsigned system_language; /* CFG_Language number, picks the puzzle names */
} ModelEnv;

typedef struct {
    ModelEnv env;
    ModelStatus status;

    Settings settings;
    unsigned settings_bad; /* values in settings.ini that were ignored */

    bool has_data; /* both files read with their exact size */
    uint8_t *meet; /* MEET_SAVE_SIZE bytes, as last read */
    uint8_t coin[GAMECOIN_SIZE];
    uint16_t coins;

    /* SpotPass puzzles listed by the PNEL files on the console. */
    PnelSet pnel;
    PnelLoadResult pnel_status;

    PanelState panels[PANEL_COUNT]; /* by panel id */
    uint8_t gallery[PANEL_COUNT];   /* panel ids in gallery order (S2) */

    /* MODEL_INTERRUPTED: what the journal says. */
    RecoveryStatus recovery;
    RecoveryInfo recovery_info;

    /* Outcome of the last purchase, restore or E-06 decision. */
    TxnReport last;
} Model;

/*
 * Reads settings, checks for an interrupted change, reads the game data and
 * looks for the original backup. Status precedence: ERR_READ, INTERRUPTED,
 * READ_ONLY, NEED_ORIGINAL, READY. Call model_free afterwards in any case.
 */
ModelStatus model_load(Model *m, const ModelEnv *env);
void model_free(Model *m);

/* Reads the game data again and updates panels, coins and status. */
ModelStatus model_refresh(Model *m);

/* S1. Allowed whenever the game data was read with the right sizes. */
BackupResult model_create_original(Model *m);

const PanelState *model_panel(const Model *m, unsigned id);

/*
 * Whether the console has this panel, given the pieces owned, when no PNEL
 * file is known. Standard panels always; SpotPass panels only with at least
 * one piece. model_load uses the PNEL index instead (pnel_index_panel_present)
 * as soon as it has found one.
 */
bool model_panel_present(unsigned id, uint64_t owned);

/*
 * Reads the picture block (a CGFX file, unpacked if it is LZ11 packed)
 * of a SpotPass panel into buf.
 * STORE_ERR_NOT_FOUND if the PNEL files do not list the panel.
 */
StoreResult model_panel_image(const Model *m, unsigned id, uint8_t *buf, size_t cap, size_t *len);

/* How many of the 7 built-in panels have no picture file on the SD card yet. */
unsigned model_builtin_pictures_missing(const Model *m);

/* True if model_panel_image may find a picture: a PNEL entry, or a built-in panel. */
bool model_panel_has_image(const Model *m, unsigned id);

/* Pieces of a panel the shop sells with the current prices. */
uint64_t model_buyable(const Model *m, unsigned id);

/* S3/S4: cost and plan for the selected pieces with the current coins and prices. */
ShopResult model_plan(const Model *m, unsigned panel, uint64_t pieces, PurchasePlan *out);

/*
 * S5. Only in MODEL_READY; otherwise nothing happens and the result is
 * TXN_ERR_REJECTED with reject SHOP_ERR_DATA. Refreshes afterwards. After
 * TXN_ERR_ROLLBACK (E-05) the journal is checked at once: if the data is
 * mixed, the status becomes MODEL_INTERRUPTED until a backup is restored.
 */
TxnResult model_buy(Model *m, const PurchasePlan *plan);

/* S7. Backs up the current state first. Not in MODEL_ERR_READ. */
TxnResult model_restore(Model *m, const char *backup);

/*
 * E-06. restore: bring back the backup named in the journal (only if the
 * journal is readable; for a damaged one pick a backup with model_restore).
 * Otherwise keep the current data and remove the journal.
 */
TxnResult model_resolve_interrupted(Model *m, bool restore);

/* S7: complete backups, newest first, original last; returns the total. */
unsigned model_backups(const Model *m, BackupInfo *out, unsigned max);

/* S8: writes settings.ini and uses the new values. */
bool model_save_settings(Model *m, const Settings *s);

#endif
