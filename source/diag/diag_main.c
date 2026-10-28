/*
 * Puzzle Swap Shop, diagnosis build.
 *
 * A text console tool for hardware tests. It is not part of
 * the user interface: its texts are plain ASCII German right here instead of
 * strings.c, and the real app never shows them. Every write to game data goes
 * through transaction.c, like in the app.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <3ds.h>

#include "app/model.h"
#include "app/pnel_load.h"
#include "core/bmp.h"
#include "core/cgfx.h"
#include "core/crc32.h"
#include "core/gamecoin.h"
#include "core/lang_map.h"
#include "core/meet_save.h"
#include "core/panel_bits.h"
#include "core/panel_catalog.h"
#include "core/pica_tex.h"
#include "core/shop.h"
#include "core/version.h"
#include "platform/backup.h"
#include "platform/clock.h"
#include "platform/fault_storage.h"
#include "platform/fs_archives.h"
#include "platform/fs_dump.h"
#include "platform/journal.h"
#include "platform/language.h"
#include "platform/title_detect.h"
#include "platform/transaction.h"

#define MAX_BACKUPS 40
#define BACKUP_ROWS 22
#define DUMP_DIR    "dump"
#define DUMP_LIST   "dump/list.txt"
#define CLEAR       "\x1b[2J\x1b[1;1H"

typedef enum {
    VIEW_MENU,
    VIEW_PIECES,
    VIEW_FAULTS,
    VIEW_BACKUPS,
    VIEW_CONFIRM,
    VIEW_RECOVERY,
} View;

typedef enum {
    ACT_INFO,
    ACT_MODEL,
    ACT_COINS,
    ACT_PIECES,
    ACT_TEST_BUY,
    ACT_INTERRUPT,
    ACT_FAULTS,
    ACT_BACKUPS,
    ACT_LIST,
    ACT_COPY,
    ACT_PNEL,
    ACT_ROMFS,
    ACT_COUNT,
} Action;

static const char *const menu_labels[ACT_COUNT] = {
    "Info",
    "App-Modell laden",
    "Muenzen -2",
    "Teil setzen/entfernen",
    "Testkauf",
    "Unterbrechung simulieren",
    "Fehler simulieren",
    "Sicherung wiederherstellen",
    "Archive auflisten",
    "Archive auf SD kopieren",
    "PNEL und Bilder lesen",
    "Mii-Plaza-RomFS pruefen",
};

/* What each fault scenario must end in; the same table as tests/test_fault_storage.c. */
static const TxnResult fault_expected[FAULT_COUNT] = {
    TXN_OK,          TXN_ERR_BACKUP,  TXN_ERR_JOURNAL, TXN_ROLLED_BACK,
    TXN_ROLLED_BACK, TXN_ROLLED_BACK, TXN_ROLLED_BACK, TXN_ERR_ROLLBACK,
};

static struct {
    PrintConsole top, bottom;
    TitleInfo title;
    FsArchives fs;
    Storage storage;
    bool fs_ok;

    uint8_t *meet;
    uint8_t coin[GAMECOIN_SIZE];
    bool data_ok; /* both files read with their exact size */
    uint16_t coins;

    View view;
    int menu;
    unsigned panel, piece;
    FaultScenario fault;
    BackupInfo backups[MAX_BACKUPS];
    unsigned backup_count, backup_sel;

    /* Pending write, run when the user confirms with A. */
    void (*confirm_fn)(void);
    char confirm_text[160];
    View confirm_back;

    RecoveryStatus recovery;
    RecoveryInfo recovery_info;
} g;

static void on_top(void)
{
    consoleSelect(&g.top);
}

static void on_bottom(void)
{
    consoleSelect(&g.bottom);
}

static void top_header(const char *title)
{
    on_top();
    printf(CLEAR "Puzzle Swap Shop Diagnose %s\n", version_string());
    printf("== %s ==\n\n", title);
}

static void wait_for_draw(void)
{
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
}

/* ---- game data ------------------------------------------------------------ */

static void reload(void)
{
    size_t ml = 0, cl = 0;
    g.data_ok = false;
    g.coins = 0;
    if (!g.fs_ok)
        return;
    if (g.storage.game_read(g.storage.ctx, GAME_MEET, g.meet, MEET_SAVE_SIZE, &ml) != STORE_OK ||
        g.storage.game_read(g.storage.ctx, GAME_COIN, g.coin, GAMECOIN_SIZE, &cl) != STORE_OK)
        return;
    g.data_ok = ml == MEET_SAVE_SIZE && cl == GAMECOIN_SIZE;
    if (g.data_ok)
        gamecoin_get(g.coin, GAMECOIN_SIZE, &g.coins);
}

static uint64_t owned_of(unsigned panel)
{
    uint64_t owned = 0;
    if (g.data_ok)
        meet_save_get_owned(g.meet, MEET_SAVE_SIZE, panel, &owned);
    return owned;
}

static void make_env(TxnEnv *env, const Storage *s)
{
    memset(env, 0, sizeof(*env));
    env->storage = s;
    env->title_id = g.title.title->title_id;
    if (!clock_now(&env->now))
        env->now = (Timestamp){2000, 1, 1, 0, 0, 0};
    env->app_version = version_string();
}

static bool need_data(void)
{
    if (g.data_ok)
        return true;
    printf("Spielstand nicht lesbar, Schreiben gesperrt.\n");
    if (g.fs_ok)
        printf("Letzter Fehler: %08lX\n", (unsigned long)g.fs.last_error);
    return false;
}

/* Prints the outcome of a transaction and the game data afterwards. */
static void report(TxnResult r, const TxnReport *rep)
{
    printf("Ergebnis: %s (Schritt %d)\n", transaction_result_name(r), (int)rep->step);
    if (r == TXN_ERR_REJECTED)
        printf("Abgelehnt mit Code %d\n", rep->reject);
    if (rep->backup[0])
        printf("Sicherung: %s\n", rep->backup);
    printf("meet.dat geschrieben: %s, gamecoin.dat: %s\n", rep->meet_written ? "ja" : "nein",
           rep->coin_written ? "ja" : "nein");
    if (rep->cleanup_failed)
        printf("Journal konnte nicht geloescht werden.\n");
    reload();
    if (g.data_ok)
        printf("Muenzen jetzt: %u\n", g.coins);
    else
        printf("Spielstand jetzt nicht lesbar! Fehler %08lX\n", (unsigned long)g.fs.last_error);
}

static TxnResult run_locked(TxnResult (*fn)(const TxnEnv *, const void *, TxnReport *),
                            const TxnEnv *env, const void *arg, TxnReport *rep)
{
    /* No HOME menu while game data is written. */
    aptSetHomeAllowed(false);
    TxnResult r = fn(env, arg, rep);
    aptSetHomeAllowed(true);
    return r;
}

/* ---- original backup and recovery ---------------------------------------- */

static void ensure_original(void)
{
    if (!g.data_ok || backup_exists(&g.storage, BACKUP_ORIGINAL))
        return;
    Manifest m;
    memset(&m, 0, sizeof(m));
    strncpy(m.app_version, version_string(), sizeof(m.app_version) - 1);
    if (!clock_now(&m.created))
        m.created = (Timestamp){2000, 1, 1, 0, 0, 0};
    m.reason = BACKUP_REASON_ORIGINAL;
    m.title_id = g.title.title->title_id;
    BackupResult r = backup_create(&g.storage, BACKUP_ORIGINAL, &m, g.meet, MEET_SAVE_SIZE, g.coin,
                                   GAMECOIN_SIZE, NULL);
    printf("Ursicherung angelegt: %s\n", r == BACKUP_OK ? "ja" : "NEIN");
    if (r != BACKUP_OK)
        printf("  Fehler %d, letzter Systemfehler %08lX\n", (int)r, (unsigned long)g.fs.last_error);
}

static void check_recovery(void)
{
    TxnEnv env;
    make_env(&env, &g.storage);
    g.recovery = transaction_check_recovery(&env, &g.recovery_info);
    switch (g.recovery) {
    case RECOVERY_NONE:
        break;
    case RECOVERY_CLEANED:
        printf("Journal-Rest gefunden und geloescht\n(Vorgang war unberuehrt oder fertig).\n");
        break;
    case RECOVERY_ERR_IO:
        printf("Journal oder Spielstand nicht lesbar: %08lX\n", (unsigned long)g.fs.last_error);
        break;
    case RECOVERY_INTERRUPTED:
    case RECOVERY_DAMAGED:
        g.view = VIEW_RECOVERY;
        break;
    }
}

static void draw_recovery(void)
{
    const JournalRecord *rec = &g.recovery_info.rec;
    top_header("Unterbrochener Vorgang (E-06)");
    if (g.recovery == RECOVERY_INTERRUPTED) {
        printf("Vorgang: %s\n", journal_op_name(rec->op));
        printf("Sicherung im Journal: %s\n\n", rec->backup);
        printf("meet.dat:     vorher %s, nachher %s\n",
               g.recovery_info.meet_is_before ? "ja" : "nein",
               g.recovery_info.meet_is_after ? "ja" : "nein");
        printf("gamecoin.dat: vorher %s, nachher %s\n\n",
               g.recovery_info.coin_is_before ? "ja" : "nein",
               g.recovery_info.coin_is_after ? "ja" : "nein");
        printf("A: Sicherung %s wiederherstellen\n", rec->backup);
    } else {
        printf("Das Journal ist beschaedigt, und der Spielstand\n");
        printf("passt nicht zur neuesten Sicherung.\n\n");
        printf("A: Liste der Sicherungen\n");
    }
    printf("B: Stand behalten, Journal loeschen\n");
}

static TxnResult restore_fn(const TxnEnv *env, const void *arg, TxnReport *rep)
{
    return transaction_restore(env, arg, rep);
}

static void recovery_choice(bool restore)
{
    top_header("Unterbrochener Vorgang (E-06)");
    if (!restore) {
        StoreResult r = journal_clear(&g.storage);
        printf("Journal geloescht: %s\n", r == STORE_OK ? "ja" : "NEIN");
    } else if (g.recovery == RECOVERY_INTERRUPTED) {
        TxnEnv env;
        TxnReport rep;
        make_env(&env, &g.storage);
        printf("Stelle %s wieder her ...\n", g.recovery_info.rec.backup);
        wait_for_draw();
        report(run_locked(restore_fn, &env, g.recovery_info.rec.backup, &rep), &rep);
    } else {
        g.view = VIEW_BACKUPS;
        g.backup_count = backup_list(&g.storage, g.backups, MAX_BACKUPS);
        if (g.backup_count > MAX_BACKUPS)
            g.backup_count = MAX_BACKUPS;
        g.backup_sel = 0;
        return;
    }
    g.view = VIEW_MENU;
    reload();
    ensure_original();
}

/* ---- confirmation --------------------------------------------------------- */

static void ask(void (*fn)(void), const char *text)
{
    g.confirm_fn = fn;
    snprintf(g.confirm_text, sizeof(g.confirm_text), "%s", text);
    g.confirm_back = g.view;
    g.view = VIEW_CONFIRM;
}

/* ---- info ------------------------------------------------------------------ */

static void show_info(void)
{
    static const char *const regions[] = {"JPN", "USA", "EUR", "AUS", "CHN", "KOR", "TWN"};
    const PlazaTitle *t = g.title.title;
    int cr = g.title.console_region;
    top_header("Info");
    printf("Konsole: %s, Mii Plaza: %s\n", cr >= 0 && cr < 7 ? regions[cr] : "?", t->code);
    printf("Titel %016llX  Save %08lX\n", (unsigned long long)t->title_id,
           (unsigned long)t->save_id);

    uint64_t ms = 0, cs = 0;
    StoreResult a = fs_archives_game_size(&g.fs, GAME_MEET, &ms);
    StoreResult b = fs_archives_game_size(&g.fs, GAME_COIN, &cs);
    printf("meet.dat: %s %llu  gamecoin.dat: %s %llu\n", a == STORE_OK ? "" : "FEHLER",
           (unsigned long long)ms, b == STORE_OK ? "" : "FEHLER", (unsigned long long)cs);
    if (a != STORE_OK || b != STORE_OK)
        printf("Letzter Fehler: %08lX\n", (unsigned long)g.fs.last_error);
    reload();
    if (!g.data_ok) {
        printf("Spielstand nicht lesbar.\n");
        return;
    }
    printf("Muenzen: %u  Pruefung meet: %d, coin: %d (0 = ok)\n", g.coins,
           (int)meet_save_validate(g.meet, MEET_SAVE_SIZE),
           (int)gamecoin_validate(g.coin, GAMECOIN_SIZE));
    BackupInfo newest;
    unsigned n = backup_list(&g.storage, &newest, 1);
    printf("Sicherungen: %u%s%s\n\n", n, n ? ", neueste " : "", n ? newest.name : "");

    printf("Panel Teile (vorhanden/gesamt):\n");
    for (unsigned id = 0; id < PANEL_COUNT; id++) {
        const PanelInfo *info = panel_catalog_get(id);
        printf("%02u %2u/%-2u  ", id, panel_bits_count(owned_of(id)), info->piece_count);
        if (id % 4 == 3)
            printf("\n");
    }
    printf("\n");
}

/* ---- coins ----------------------------------------------------------------- */

static int spend_two(uint8_t *meet, size_t meet_len, uint8_t *coin, size_t coin_len, void *arg)
{
    (void)meet;
    (void)meet_len;
    (void)arg;
    return gamecoin_spend(coin, coin_len, 2) == COIN_OK ? 0 : 1;
}

typedef struct {
    TxnChange fn;
    void *arg;
} ChangeArg;

static TxnResult change_fn(const TxnEnv *env, const void *arg, TxnReport *rep)
{
    const ChangeArg *c = arg;
    return transaction_change(env, c->fn, c->arg, rep);
}

static void run_change(TxnChange fn, void *arg)
{
    ChangeArg c = {fn, arg};
    TxnEnv env;
    TxnReport rep;
    make_env(&env, &g.storage);
    report(run_locked(change_fn, &env, &c, &rep), &rep);
}

static void do_coins(void)
{
    top_header("Muenzen -2");
    reload();
    if (!need_data())
        return;
    printf("Muenzen vorher: %u\n", g.coins);
    run_change(spend_two, NULL);
}

/* ---- one piece ------------------------------------------------------------- */

typedef struct {
    unsigned panel;
    uint64_t piece;
    bool add;
} Toggle;

static int toggle_piece(uint8_t *meet, size_t meet_len, uint8_t *coin, size_t coin_len, void *arg)
{
    const Toggle *t = arg;
    (void)coin;
    (void)coin_len;
    MeetResult r = t->add ? meet_save_add_pieces(meet, meet_len, t->panel, t->piece)
                          : meet_save_remove_pieces(meet, meet_len, t->panel, t->piece);
    return r == MEET_OK ? 0 : (int)r;
}

static void draw_pieces(void)
{
    const PanelInfo *info = panel_catalog_get(g.panel);
    unsigned count = info->piece_count;
    unsigned cols = panel_catalog_columns(count);
    uint64_t owned = owned_of(g.panel);
    bool have = (owned >> g.piece) & 1;

    top_header("Teil setzen/entfernen");
    printf("Panel %u, %s, %u Teile, %u vorhanden\n", g.panel,
           info->kind == PANEL_STANDARD ? "Standard" : "SpotPass", count, panel_bits_count(owned));
    printf("%.49s\n", info->fallback_name);
    printf("Raster %u x %u, Teil 0 oben links, zeilenweise\n\n", cols, count / cols);
    for (unsigned i = 0; i < count; i++) {
        const char *cell = (owned >> i) & 1 ? "##" : "..";
        printf(i == g.piece ? "[%s]" : " %s ", cell);
        if (i % cols == cols - 1)
            printf("\n");
    }
    printf("\nTeil %u (Zeile %u, Spalte %u): %s\n\n", g.piece, g.piece / cols + 1,
           g.piece % cols + 1, have ? "vorhanden" : "fehlt");
    printf("Fehlend:");
    for (unsigned i = 0; i < count; i++) {
        if (!((owned >> i) & 1))
            printf(" %u", i);
    }
    printf("\n");
    if (!g.data_ok)
        printf("\nSpielstand nicht lesbar.\n");
}

static void do_toggle(void)
{
    uint64_t owned = owned_of(g.panel);
    Toggle t = {g.panel, (uint64_t)1 << g.piece, !((owned >> g.piece) & 1)};
    top_header(t.add ? "Teil setzen" : "Teil entfernen");
    if (!need_data())
        return;
    printf("Panel %u, Teil %u\n", g.panel, g.piece);
    run_change(toggle_piece, &t);
    owned = owned_of(g.panel);
    printf("Teil %u jetzt: %s\n", g.piece, (owned >> g.piece) & 1 ? "vorhanden" : "fehlt");
}

/* ---- test purchase, interruption, faults ----------------------------------- */

/*
 * The test purchase takes the lowest missing piece of the first incomplete
 * panel that the shop would sell, standard or SpotPass (model_panel_present).
 */
static ShopResult test_plan(PurchasePlan *plan)
{
    ShopPrices prices = shop_default_prices();
    for (unsigned id = 0; id < PANEL_COUNT; id++) {
        PanelState p;
        memset(&p, 0, sizeof(p));
        p.id = (uint8_t)id;
        p.piece_count = panel_catalog_get(id)->piece_count;
        p.owned = owned_of(id);
        p.available = model_panel_present(id, p.owned);
        if (!p.available)
            continue;
        uint64_t missing = shop_missing(&p);
        if (missing != 0)
            return shop_plan_build(&p, missing & (~missing + 1), g.coins, &prices, plan);
    }
    return SHOP_ERR_EMPTY;
}

typedef struct {
    const PurchasePlan *plan;
} BuyArg;

static TxnResult buy_fn(const TxnEnv *env, const void *arg, TxnReport *rep)
{
    return transaction_purchase(env, ((const BuyArg *)arg)->plan, rep);
}

/* Runs the test purchase over storage s; returns false if there was nothing to buy. */
static bool test_buy(const Storage *s, TxnStep stop_after, TxnResult *out)
{
    PurchasePlan plan;
    reload();
    if (!need_data())
        return false;
    ShopResult sr = test_plan(&plan);
    if (sr != SHOP_OK) {
        printf("Kein Testkauf moeglich (Code %d).\n", (int)sr);
        printf("Braucht ein unvollstaendiges Puzzle\nund mindestens 2 Muenzen.\n");
        if (sr == SHOP_ERR_EMPTY)
            printf("Sind alle fertig: bei \"Teil setzen/entfernen\"\nein Teil entfernen, dann "
                   "nochmal.\n");
        return false;
    }
    unsigned piece = 0;
    while (!((plan.pieces >> piece) & 1))
        piece++;
    printf("Kaufe Panel %u, Teil %u fuer %u Muenzen\n(Muenzen vorher: %u)\n\n", plan.panel_id,
           piece, plan.cost, g.coins);
    wait_for_draw();

    TxnEnv env;
    TxnReport rep;
    BuyArg arg = {&plan};
    make_env(&env, s);
    env.stop_after = stop_after;
    *out = run_locked(buy_fn, &env, &arg, &rep);
    report(*out, &rep);
    printf("Teil %u jetzt: %s\n", piece,
           (owned_of(plan.panel_id) >> piece) & 1 ? "vorhanden" : "fehlt");
    return true;
}

/* ---- the app model without UI ---------------------------------------------- */

static Model s_model;

static const char *const model_status_names[] = {
    "READY (S2)", "NEED_ORIGINAL (S1)", "READ_ONLY (E-03)", "INTERRUPTED (E-06)", "ERR_READ (E-01)",
};

static unsigned system_language(void)
{
    unsigned code = LANG_MAP_SYS_EN;
    language_system_code(&code);
    return code;
}

static bool load_model(void)
{
    ModelEnv env = {&g.storage, g.title.title, clock_now, version_string(), system_language()};
    model_free(&s_model);
    ModelStatus st = model_load(&s_model, &env);
    printf("Status: %s\n", model_status_names[st]);
    return st == MODEL_READY;
}

static void show_model(void)
{
    static const char *const languages[] = {"auto", "de", "en"};
    top_header("App-Modell");
    load_model();
    const Settings *set = &s_model.settings;
    printf("Muenzen %u, Preis %u, selten %u, Klang %s, Sprache %s\n", s_model.coins,
           set->prices.per_piece, set->prices.per_rare_piece, set->sound ? "an" : "aus",
           languages[set->language]);
    if (s_model.settings_bad)
        printf("settings.ini: %u Werte ignoriert\n", s_model.settings_bad);
    printf("\nGalerie (Reihenfolge, vorhanden/gesamt, kaufbar):\n");
    for (unsigned i = 0; i < PANEL_COUNT; i++) {
        const PanelState *p = model_panel(&s_model, s_model.gallery[i]);
        if (!p->available) {
            printf("\n...dann %u nicht vorhandene SpotPass-Panels\n", PANEL_COUNT - i);
            break;
        }
        printf("%02u %2u/%-2u %2u  ", p->id, panel_bits_count(p->owned), p->piece_count,
               panel_bits_count(model_buyable(&s_model, p->id)));
        if (i % 3 == 2)
            printf("\n");
    }
    printf("\n");
}

static void do_test_buy(void)
{
    PurchasePlan plan;
    top_header("Testkauf ueber das App-Modell");
    if (!load_model()) {
        printf("Kauf nur im Status READY.\n");
        return;
    }
    /* First incomplete panel in gallery order, standard or SpotPass, its lowest missing piece. */
    unsigned panel = PANEL_COUNT;
    for (unsigned i = 0; i < PANEL_COUNT && panel == PANEL_COUNT; i++) {
        unsigned id = s_model.gallery[i];
        if (model_buyable(&s_model, id) != 0)
            panel = id;
    }
    if (panel == PANEL_COUNT) {
        printf("Kein unvollstaendiges Puzzle.\n");
        printf("Bei \"Teil setzen/entfernen\" ein Teil entfernen,\ndann nochmal.\n");
        return;
    }
    uint64_t missing = model_buyable(&s_model, panel);
    uint64_t piece = missing & (~missing + 1);
    ShopResult sr = model_plan(&s_model, panel, piece, &plan);
    if (sr != SHOP_OK) {
        printf("Kein Kauf moeglich (Code %d).\n", (int)sr);
        return;
    }
    unsigned index = 0;
    while (!((piece >> index) & 1))
        index++;
    printf("Kaufe Panel %u, Teil %u fuer %u Muenzen\n(Muenzen vorher: %u)\n\n", panel, index,
           plan.cost, s_model.coins);
    wait_for_draw();

    aptSetHomeAllowed(false);
    TxnResult r = model_buy(&s_model, &plan);
    aptSetHomeAllowed(true);
    report(r, &s_model.last);
    printf("Modell: %u Muenzen, Teil %u %s, Status %s\n", s_model.coins, index,
           (model_panel(&s_model, panel)->owned & piece) ? "vorhanden" : "fehlt",
           model_status_names[s_model.status]);
}

static void do_interrupt(void)
{
    TxnResult r;
    top_header("Unterbrechung simulieren");
    if (!test_buy(&g.storage, TXN_STEP_MEET, &r))
        return;
    printf("\nAbbruch nach Schritt 5: Teil geschrieben,\n");
    printf("Muenzen nicht, Journal bleibt liegen.\n");
    printf("Jetzt mit START beenden und neu starten.\n");
    printf("Erwartet: Meldung E-06.\n");
}

static void do_fault(void)
{
    FaultStorage f;
    TxnResult r;
    top_header("Fehler simulieren");
    printf("Fall: %s\n", fault_scenario_name(g.fault));
    printf("Erwartet: %s\n", transaction_result_name(fault_expected[g.fault]));
    reload();
    uint32_t meet_crc = g.data_ok ? crc32(g.meet, MEET_SAVE_SIZE) : 0;
    uint32_t coin_crc = g.data_ok ? crc32(g.coin, GAMECOIN_SIZE) : 0;

    fault_storage_init(&f, &g.storage, g.fault);
    if (!test_buy(&f.storage, TXN_STEP_NONE, &r))
        return;
    bool same = g.data_ok && crc32(g.meet, MEET_SAVE_SIZE) == meet_crc &&
                crc32(g.coin, GAMECOIN_SIZE) == coin_crc;
    printf("\nFehler ausgeloest: %ux\n", f.fired);
    printf("Spielstand unveraendert: %s\n", same ? "ja" : "NEIN");
    printf("%s\n",
           r == fault_expected[g.fault] && same && f.fired > 0 ? "Wie erwartet." : "ABWEICHUNG!");
    if (g.fault == FAULT_ROLLBACK)
        printf("Das Journal bleibt; beim naechsten Start wird\nes als Rest erkannt und "
               "geloescht.\n");
}

static void draw_faults(void)
{
    top_header("Fehler simulieren");
    printf("Jeder Fall macht einen Testkauf (1 Teil, 2 Muenzen)\n");
    printf("und laesst genau einen Schritt scheitern.\n");
    printf("Danach muss der Spielstand unveraendert sein.\n\n");
    for (int s = 1; s < FAULT_COUNT; s++) {
        printf("%c %d %-34s %s\n", s == (int)g.fault ? '>' : ' ', s,
               fault_scenario_name((FaultScenario)s), transaction_result_name(fault_expected[s]));
    }
}

/* ---- backups -------------------------------------------------------------- */

static void open_backups(void)
{
    g.backup_count = backup_list(&g.storage, g.backups, MAX_BACKUPS);
    if (g.backup_count > MAX_BACKUPS)
        g.backup_count = MAX_BACKUPS;
    g.backup_sel = 0;
    g.view = VIEW_BACKUPS;
}

static void draw_backups(void)
{
    top_header("Sicherung wiederherstellen");
    if (g.backup_count == 0) {
        printf("Keine vollstaendige Sicherung gefunden.\n");
        return;
    }
    printf("Neueste oben. Anlass: original, purchase,\nrestore (vor Wiederherstellung), diag.\n\n");
    unsigned first = g.backup_sel >= BACKUP_ROWS ? g.backup_sel - BACKUP_ROWS + 1 : 0;
    for (unsigned i = first; i < g.backup_count && i < first + BACKUP_ROWS; i++) {
        printf("%c %-18s %s\n", i == g.backup_sel ? '>' : ' ', g.backups[i].name,
               manifest_reason_name(g.backups[i].manifest.reason));
    }
}

static void do_restore(void)
{
    const char *name = g.backups[g.backup_sel].name;
    TxnEnv env;
    TxnReport rep;
    top_header("Sicherung wiederherstellen");
    printf("Stelle %s wieder her ...\n", name);
    wait_for_draw();
    make_env(&env, &g.storage);
    report(run_locked(restore_fn, &env, name, &rep), &rep);
    g.backup_count = backup_list(&g.storage, g.backups, MAX_BACKUPS);
    if (g.backup_count > MAX_BACKUPS)
        g.backup_count = MAX_BACKUPS;
    g.backup_sel = 0;
}

/* ---- archives -------------------------------------------------------------- */

typedef struct {
    char *text;
    size_t len, cap;
    unsigned files;
    u64 bytes;
    unsigned shown;
} ListOut;

static void list_append(ListOut *o, const char *line)
{
    size_t n = strlen(line);
    if (o->len + n + 1 > o->cap) {
        size_t cap = o->cap ? o->cap * 2 : 4096;
        while (cap < o->len + n + 1)
            cap *= 2;
        char *grown = realloc(o->text, cap);
        if (grown == NULL)
            return;
        o->text = grown;
        o->cap = cap;
    }
    memcpy(o->text + o->len, line, n + 1);
    o->len += n;
}

static bool list_entry(const char *path, bool is_dir, u64 size, void *arg)
{
    ListOut *o = arg;
    char line[300];
    if (is_dir)
        snprintf(line, sizeof(line), "  %s/\n", path);
    else
        snprintf(line, sizeof(line), "  %s  %llu\n", path, (unsigned long long)size);
    list_append(o, line);
    if (!is_dir) {
        o->files++;
        o->bytes += size;
    }
    if (o->shown < 4) {
        printf("  %.46s\n", path);
        o->shown++;
    }
    return true;
}

static void write_list(const char *text, size_t len)
{
    StoreResult r = g.storage.sd_mkdir(g.storage.ctx, DUMP_DIR);
    if (r == STORE_OK || r == STORE_ERR_EXISTS)
        r = g.storage.sd_write(g.storage.ctx, DUMP_LIST, (const uint8_t *)text, len);
    printf("\nListe in sdmc:%s/%s: %s\n", FS_APP_DIR, DUMP_LIST, r == STORE_OK ? "ja" : "NEIN");
}

static void do_list(void)
{
    ListOut o;
    memset(&o, 0, sizeof(o));
    top_header("Archive auflisten");
    for (int a = 0; a < DUMP_ARCHIVE_COUNT; a++) {
        char line[96];
        u32 id = 0;
        unsigned files_before = o.files;
        u64 bytes_before = o.bytes;
        o.shown = 0;
        printf("%s:\n", fs_dump_label((DumpArchive)a));
        snprintf(line, sizeof(line), "%s:\n", fs_dump_label((DumpArchive)a));
        list_append(&o, line);
        Result res = fs_dump_walk(g.title.title, (DumpArchive)a, list_entry, &o, &id);
        if (R_FAILED(res))
            snprintf(line, sizeof(line), "  Fehler %08lX (Id %08lX)\n", (unsigned long)res,
                     (unsigned long)id);
        else
            snprintf(line, sizeof(line), "  %u Dateien, %llu Bytes (Id %08lX)\n",
                     o.files - files_before, (unsigned long long)(o.bytes - bytes_before),
                     (unsigned long)id);
        list_append(&o, line);
        printf("%s", line);
    }
    if (o.text != NULL)
        write_list(o.text, o.len);
    free(o.text);
}

typedef struct {
    char (*paths)[256];
    unsigned count, cap;
} PathList;

static bool collect_path(const char *path, bool is_dir, u64 size, void *arg)
{
    PathList *l = arg;
    (void)size;
    if (l->count == l->cap) {
        unsigned cap = l->cap ? l->cap * 2 : 32;
        void *grown = realloc(l->paths, cap * sizeof(*l->paths));
        if (grown == NULL)
            return false;
        l->paths = grown;
        l->cap = cap;
    }
    /* Folders are marked with a trailing '/' so they are created in order. */
    snprintf(l->paths[l->count++], sizeof(*l->paths), "%s%s", path, is_dir ? "/" : "");
    return true;
}

static void do_copy(void)
{
    unsigned copied = 0, failed = 0;
    top_header("Archive auf SD kopieren");
    StoreResult sr = g.storage.sd_mkdir(g.storage.ctx, DUMP_DIR);
    if (sr != STORE_OK && sr != STORE_ERR_EXISTS) {
        printf("Ordner %s nicht anlegbar.\n", DUMP_DIR);
        return;
    }
    for (int a = 0; a < DUMP_ARCHIVE_COUNT; a++) {
        const char *label = fs_dump_label((DumpArchive)a);
        char dest[300];
        PathList l = {NULL, 0, 0};
        printf("%s ... ", label);
        wait_for_draw();
        Result res = fs_dump_walk(g.title.title, (DumpArchive)a, collect_path, &l, NULL);
        if (R_FAILED(res)) {
            printf("Fehler %08lX\n", (unsigned long)res);
            free(l.paths);
            continue;
        }
        snprintf(dest, sizeof(dest), "%s/%s", DUMP_DIR, label);
        g.storage.sd_mkdir(g.storage.ctx, dest);
        unsigned here = 0;
        for (unsigned i = 0; i < l.count; i++) {
            size_t n = strlen(l.paths[i]);
            snprintf(dest, sizeof(dest), "%s/%s%s", DUMP_DIR, label, l.paths[i]);
            if (n > 0 && l.paths[i][n - 1] == '/') {
                dest[strlen(dest) - 1] = '\0';
                g.storage.sd_mkdir(g.storage.ctx, dest);
                continue;
            }
            u8 *data = NULL;
            u32 len = 0;
            res = fs_dump_read(g.title.title, (DumpArchive)a, l.paths[i], &data, &len);
            if (R_SUCCEEDED(res) &&
                g.storage.sd_write(g.storage.ctx, dest, data, len) == STORE_OK) {
                copied++;
                here++;
            } else {
                failed++;
            }
            free(data);
        }
        printf("%u Dateien\n", here);
        free(l.paths);
    }
    printf("\nKopiert: %u, Fehler: %u\n", copied, failed);
    printf("Ziel: sdmc:%s/%s/\n", FS_APP_DIR, DUMP_DIR);
    printf("Die Dateien enthalten persoenliche Daten\nund bleiben auf deinem PC.\n");
}

/* ---- PNEL and puzzle pictures ---------------------------------------------- */

#define PNEL_IMAGE_CAP (2u * 1024 * 1024)

static void hex_line(const uint8_t *p, size_t n)
{
    for (size_t i = 0; i < n; i++)
        printf("%02X", p[i]);
    printf("\n");
}

/* Decodes one picture, prints what was found and writes cgfx and bmp to the dump folder. */
static void pnel_try_image(unsigned id, uint8_t *buf, size_t cap)
{
    char path[300];
    size_t len = 0;
    StoreResult sr = model_panel_image(&s_model, id, buf, cap, &len);
    printf("\nBild Panel %u: %u Bytes (Ergebnis %d)\n", id, (unsigned)len, (int)sr);
    if (sr != STORE_OK)
        return;
    snprintf(path, sizeof(path), "%s/pnel-%02u.cgfx", DUMP_DIR, id);
    g.storage.sd_write(g.storage.ctx, path, buf, len);

    CgfxTexture tex;
    CgfxResult cr = cgfx_find_texture(buf, len, &tex);
    if (cr != CGFX_OK) {
        printf("CGFX Ergebnis %d. Anfang der Datei:\n", (int)cr);
        hex_line(buf, len < 24 ? len : 24);
        /* The texture object, if there is one, so the layout can be corrected. */
        for (size_t p = 4; p + 4 <= len; p += 4) {
            if (memcmp(buf + p, "TXOB", 4) == 0) {
                printf("TXOB bei 0x%X, 0x50 Bytes:\n", (unsigned)(p - 4));
                for (size_t k = 0; k < 0x50 && p - 4 + k + 16 <= len; k += 16)
                    hex_line(buf + p - 4 + k, 16);
                break;
            }
        }
        return;
    }
    printf("Textur %ux%u, Format %u, Daten %u Bytes\n", (unsigned)tex.width, (unsigned)tex.height,
           (unsigned)tex.format, (unsigned)tex.data_size);
    size_t rgba_len = (size_t)tex.width * tex.height * 4;
    uint8_t *rgba = malloc(rgba_len);
    size_t bmp_len = bmp_size(tex.width, tex.height);
    uint8_t *bmp = malloc(bmp_len ? bmp_len : 1);
    if (rgba != NULL && bmp != NULL && cgfx_decode(buf, len, &tex, rgba) == CGFX_OK &&
        bmp_encode(tex.width, tex.height, rgba, bmp, bmp_len) == bmp_len) {
        snprintf(path, sizeof(path), "%s/pnel-%02u.bmp", DUMP_DIR, id);
        sr = g.storage.sd_write(g.storage.ctx, path, bmp, bmp_len);
        printf("Bild decodiert, BMP geschrieben (%d)\n", (int)sr);
    } else {
        printf("Decodieren fehlgeschlagen\n");
    }
    free(rgba);
    free(bmp);
}

static bool probe_file(const char *name, bool is_dir, uint64_t size, void *arg)
{
    unsigned *shown = arg;
    uint8_t head[32];
    size_t got = 0;
    (void)is_dir;
    if ((*shown)++ >= 4)
        return false;
    printf("%s: %lu Bytes\n", name, (unsigned long)size);
    if (g.storage.boss_read(g.storage.ctx, name, 0, head, sizeof(head), &got) == STORE_OK)
        hex_line(head, got);
    return true;
}

/* Shows size and first bytes of the first files of the BOSS archive, so a wrapped PNEL can be seen.
 */
static void probe_boss(void)
{
    unsigned shown = 0;
    if (g.storage.boss_list == NULL ||
        g.storage.boss_list(g.storage.ctx, probe_file, &shown) != STORE_OK)
        printf("BOSS-Archiv nicht lesbar\n");
}

/* All 12 name slots of one entry, to see which slot holds which language. */
/* Size and format of every picture, and each one saved as pnel-NN.cgfx. */
static void print_textures(uint8_t *buf, size_t cap)
{
    printf("\nTexturen (Panel:BxH/Format):\n");
    unsigned col = 0;
    for (unsigned id = 0; id < PANEL_COUNT; id++) {
        if (!s_model.pnel.index.panel[id].present)
            continue;
        size_t len = 0;
        CgfxTexture tex;
        if (model_panel_image(&s_model, id, buf, cap, &len) != STORE_OK ||
            cgfx_find_texture(buf, len, &tex) != CGFX_OK) {
            printf("%2u:?  ", id);
        } else {
            char path[300];
            snprintf(path, sizeof(path), "%s/pnel-%02u.cgfx", DUMP_DIR, id);
            g.storage.sd_write(g.storage.ctx, path, buf, len);
            printf("%2u:%ux%u/%u  ", id, (unsigned)tex.width, (unsigned)tex.height,
                   (unsigned)tex.format);
        }
        if (++col % 3 == 0)
            printf("\n");
    }
    printf("\n");
}

#define PNEL_SETTINGS_DUMP_MAX 0x10000u

/* Raw header and entries of the first PNEL file, to find where the rare pieces are stored.
 */
static void dump_pnel_table(const PnelSet *set)
{
    static uint8_t table[PNEL_HEADER_SIZE + 64u * PNEL_ENTRY_SIZE];
    size_t got = 0;
    PnelHeader h;
    if (set->files == 0 ||
        g.storage.boss_read(g.storage.ctx, set->path[0], set->base[0], table, PNEL_HEADER_SIZE,
                            &got) != STORE_OK ||
        pnel_read_head(table, got, (size_t)UINT32_MAX, &h) != PNEL_OK || h.count > 64)
        return;
    size_t len = PNEL_HEADER_SIZE + (size_t)h.count * PNEL_ENTRY_SIZE;
    if (g.storage.boss_read(g.storage.ctx, set->path[0], set->base[0], table, len, &got) !=
            STORE_OK ||
        got != len)
        return;
    g.storage.sd_mkdir(g.storage.ctx, DUMP_DIR);
    g.storage.sd_write(g.storage.ctx, DUMP_DIR "/pnel-table.bin", table, len);

    /* The pattern of the rare pieces may live in the settings block of each entry. */
    static uint8_t block[PNEL_SETTINGS_DUMP_MAX];
    for (uint32_t i = 0; i < h.count; i++) {
        PnelEntry e;
        if (pnel_read_entry_at(table + PNEL_HEADER_SIZE + (size_t)i * PNEL_ENTRY_SIZE,
                               (size_t)UINT32_MAX, &e) != PNEL_OK ||
            e.settings_size == 0 || e.settings_size > sizeof(block) ||
            g.storage.boss_read(g.storage.ctx, set->path[0], set->base[0] + e.settings_offset,
                                block, e.settings_size, &got) != STORE_OK ||
            got != e.settings_size)
            continue;
        char path[48];
        snprintf(path, sizeof(path), DUMP_DIR "/pnel-%02u.set", (unsigned)e.order);
        g.storage.sd_write(g.storage.ctx, path, block, got);
    }
}

static void do_pnel(void)
{
    top_header("PNEL und Bilder");
    if (!load_model())
        printf("(Modell nicht bereit, PNEL wird trotzdem gelesen)\n");
    const PnelSet *set = &s_model.pnel;
    static const char *const status[] = {"kein Zugriff", "keine PNEL-Datei", "ok"};
    printf("PNEL: %s, geprueft %u Dateien (%u durchsucht), gefunden %u\n",
           status[s_model.pnel_status], set->candidates, set->scanned, set->files);
    probe_boss();
    for (unsigned f = 0; f < set->files; f++)
        printf("Datei %u: %s ab 0x%lX\n", f, set->path[f], (unsigned long)set->base[f]);
    printf("Eintraege: %u uebernommen, %u ignoriert\n", set->index.accepted, set->index.ignored);

    unsigned first = PANEL_COUNT;
    static char list[PANEL_COUNT * 80];
    size_t list_len = 0;
    for (unsigned id = 0; id < PANEL_COUNT; id++) {
        const PnelPanel *p = &set->index.panel[id];
        if (!p->present)
            continue;
        if (first == PANEL_COUNT)
            first = id;
        printf("%02u pink %02X img %lu  %s\n", id, (unsigned)p->pink, (unsigned long)p->image_size,
               p->name);
        list_len += (size_t)snprintf(list + list_len, sizeof(list) - list_len,
                                     "%02u pink %02X img %lu  %s\n", id, (unsigned)p->pink,
                                     (unsigned long)p->image_size, p->name);
    }
    /* The console cannot scroll; the full list goes to the SD card. */
    g.storage.sd_mkdir(g.storage.ctx, DUMP_DIR);
    g.storage.sd_write(g.storage.ctx, DUMP_DIR "/pnel-list.txt", (const uint8_t *)list, list_len);
    dump_pnel_table(set);
    if (first == PANEL_COUNT)
        return;
    g.storage.sd_mkdir(g.storage.ctx, DUMP_DIR);
    uint8_t *buf = malloc(PNEL_IMAGE_CAP);
    if (buf == NULL)
        return;
    pnel_try_image(first, buf, PNEL_IMAGE_CAP);
    print_textures(buf, PNEL_IMAGE_CAP);
    free(buf);
    printf("\nDateien: sdmc:%s/%s/pnel-*\n", FS_APP_DIR, DUMP_DIR);
}

/* ---- can the app read the RomFS of Mii Plaza? ------------------------------ */

/* Opens the RomFS of the Mii Plaza title read-only and shows the size and the first bytes. */
static void do_romfs(void)
{
    top_header("Mii-Plaza-RomFS");
    if (g.title.title == NULL) {
        printf("Mii Plaza wurde nicht erkannt.\n");
        return;
    }
    uint64_t tid = g.title.title->title_id;
    printf("Titel-ID %08lX%08lX\n", (unsigned long)(tid >> 32), (unsigned long)(tid & 0xFFFFFFFF));
    /* The NCCH archive on NAND answered 0xE0E046BE (invalid argument), SD and
     * card "not found". Now both archive kinds with both file path forms, on NAND. */
    static const struct {
        const char *name;
        u32 archive;
        bool empty_file_path;
    } tries[] = {
        {"RomFS-Archiv, Pfad bin", 3, false},
        {"RomFS-Archiv, Pfad leer", 3, true},
        {"NCCH-Archiv, Pfad bin", 0x2345678A, false},
        {"NCCH-Archiv, Pfad leer", 0x2345678A, true},
    };
    for (unsigned i = 0; i < sizeof(tries) / sizeof(tries[0]); i++) {
        u32 archive_path[4] = {(u32)(tid & 0xFFFFFFFF), (u32)(tid >> 32), MEDIATYPE_NAND, 0};
        u32 file_path[3] = {0, 0, 0}; /* 0 = RomFS */
        Handle file = 0;
        FS_Path file_fs = tries[i].empty_file_path
                              ? (FS_Path){PATH_EMPTY, 1, ""}
                              : (FS_Path){PATH_BINARY, sizeof(file_path), file_path};
        Result res = FSUSER_OpenFileDirectly(
            &file, (FS_ArchiveID)tries[i].archive,
            (FS_Path){PATH_BINARY, sizeof(archive_path), archive_path}, file_fs, FS_OPEN_READ, 0);
        printf("%s: 0x%08lX\n", tries[i].name, (unsigned long)res);
        if (R_FAILED(res))
            continue;
        u64 size = 0;
        FSFILE_GetSize(file, &size);
        u8 head[0x60];
        u32 got = 0;
        res = FSFILE_Read(file, &got, 0, head, sizeof(head));
        printf("Groesse %lu, gelesen %lu (0x%08lX)\n", (unsigned long)size, (unsigned long)got,
               (unsigned long)res);
        if (R_SUCCEEDED(res))
            for (u32 k = 0; k < got; k += 32)
                hex_line(head + k, got - k < 32 ? got - k : 32);
        FSFILE_Close(file);
    }
}

/* ---- menu and input ------------------------------------------------------- */

static void draw_bottom(void)
{
    on_bottom();
    printf(CLEAR);
    printf("Diagnose %s", version_string());
    if (g.title.title != NULL)
        printf("  %s", g.title.title->code);
    if (g.data_ok)
        printf("  %u Muenzen", g.coins);
    printf("\n----------------------------------------");

    switch (g.view) {
    case VIEW_MENU:
        for (int i = 0; i < ACT_COUNT; i++)
            printf("%c %s\n", i == g.menu ? '>' : ' ', menu_labels[i]);
        printf("\nA: ausfuehren   START: beenden\n");
        break;
    case VIEW_PIECES:
        printf("L/R: Panel wechseln\n");
        printf("Steuerkreuz: Teil waehlen\n");
        printf("A: Teil setzen oder entfernen\n");
        printf("B: zurueck\n");
        break;
    case VIEW_FAULTS:
        printf("Hoch/Runter: Fall waehlen\n");
        printf("A: Testkauf mit diesem Fehler\n");
        printf("B: zurueck\n");
        break;
    case VIEW_BACKUPS:
        printf("Hoch/Runter: Sicherung waehlen\n");
        printf("A: wiederherstellen\n");
        printf("B: zurueck\n");
        break;
    case VIEW_CONFIRM:
        printf("%s\n\nA: ja    B: nein\n", g.confirm_text);
        break;
    case VIEW_RECOVERY:
        printf("A: wiederherstellen   B: behalten\n");
        break;
    }
}

static void draw_top_view(void)
{
    switch (g.view) {
    case VIEW_PIECES:
        draw_pieces();
        break;
    case VIEW_FAULTS:
        draw_faults();
        break;
    case VIEW_BACKUPS:
        draw_backups();
        break;
    case VIEW_RECOVERY:
        draw_recovery();
        break;
    default:
        break;
    }
}

static void start_action(Action a)
{
    char text[160];
    switch (a) {
    case ACT_INFO:
        show_info();
        break;
    case ACT_MODEL:
        show_model();
        break;
    case ACT_COINS:
        ask(do_coins, "Zwei Spielmuenzen abziehen?");
        break;
    case ACT_PIECES:
        reload();
        g.view = VIEW_PIECES;
        break;
    case ACT_TEST_BUY:
        ask(do_test_buy, "Testkauf: ein Teil fuer 2 Muenzen kaufen?");
        break;
    case ACT_INTERRUPT:
        ask(do_interrupt, "Testkauf nach den Teilen abbrechen?\nDanach App neu starten.");
        break;
    case ACT_FAULTS:
        if (g.fault == FAULT_NONE)
            g.fault = FAULT_BACKUP_WRITE;
        g.view = VIEW_FAULTS;
        break;
    case ACT_BACKUPS:
        open_backups();
        break;
    case ACT_LIST:
        do_list();
        break;
    case ACT_COPY:
        snprintf(text, sizeof(text), "Archive nach sdmc:%s/%s kopieren?", FS_APP_DIR, DUMP_DIR);
        ask(do_copy, text);
        break;
    case ACT_PNEL:
        do_pnel();
        break;
    case ACT_ROMFS:
        do_romfs();
        break;
    default:
        break;
    }
}

/* Returns true if the screens need redrawing. */
static bool handle(u32 down)
{
    if (down == 0)
        return false;
    switch (g.view) {
    case VIEW_MENU:
        if (down & KEY_DOWN)
            g.menu = (g.menu + 1) % ACT_COUNT;
        else if (down & KEY_UP)
            g.menu = (g.menu + ACT_COUNT - 1) % ACT_COUNT;
        else if (down & KEY_A) {
            if (!g.fs_ok)
                return false;
            start_action((Action)g.menu);
        } else
            return false;
        return true;

    case VIEW_PIECES: {
        unsigned count = panel_catalog_get(g.panel)->piece_count;
        unsigned cols = panel_catalog_columns(count);
        if (down & KEY_B)
            g.view = VIEW_MENU;
        else if (down & (KEY_R | KEY_L)) {
            g.panel = (down & KEY_R) ? (g.panel + 1) % PANEL_COUNT
                                     : (g.panel + PANEL_COUNT - 1) % PANEL_COUNT;
            g.piece = 0;
        } else if (down & KEY_RIGHT)
            g.piece = (g.piece + 1) % count;
        else if (down & KEY_LEFT)
            g.piece = (g.piece + count - 1) % count;
        else if (down & KEY_DOWN)
            g.piece = (g.piece + cols) % count;
        else if (down & KEY_UP)
            g.piece = (g.piece + count - cols) % count;
        else if (down & KEY_A) {
            char text[160];
            bool have = (owned_of(g.panel) >> g.piece) & 1;
            snprintf(text, sizeof(text), "Panel %u, Teil %u %s?", g.panel, g.piece,
                     have ? "entfernen" : "setzen");
            ask(do_toggle, text);
        } else
            return false;
        return true;
    }

    case VIEW_FAULTS:
        if (down & KEY_B)
            g.view = VIEW_MENU;
        else if (down & KEY_DOWN)
            g.fault = g.fault + 1 < FAULT_COUNT ? g.fault + 1 : FAULT_BACKUP_WRITE;
        else if (down & KEY_UP)
            g.fault = g.fault > FAULT_BACKUP_WRITE ? g.fault - 1 : FAULT_COUNT - 1;
        else if (down & KEY_A)
            ask(do_fault, "Testkauf mit diesem Fehler ausfuehren?");
        else
            return false;
        return true;

    case VIEW_BACKUPS:
        if (down & KEY_B)
            g.view = VIEW_MENU;
        else if ((down & KEY_DOWN) && g.backup_sel + 1 < g.backup_count)
            g.backup_sel++;
        else if ((down & KEY_UP) && g.backup_sel > 0)
            g.backup_sel--;
        else if ((down & KEY_A) && g.backup_count > 0) {
            char text[160];
            snprintf(text, sizeof(text), "Sicherung %s wiederherstellen?",
                     g.backups[g.backup_sel].name);
            ask(do_restore, text);
        } else
            return false;
        return true;

    case VIEW_CONFIRM:
        if (down & KEY_A) {
            /* The action prints its result on the top screen; keep it until the next key. */
            g.view = g.confirm_back;
            g.confirm_fn();
            draw_bottom();
            return false;
        }
        if (down & KEY_B) {
            g.view = g.confirm_back;
            return true;
        }
        return false;

    case VIEW_RECOVERY:
        if (down & (KEY_A | KEY_B)) {
            recovery_choice((down & KEY_A) != 0);
            return true;
        }
        return false;
    }
    return false;
}

static void boot(void)
{
    top_header("Start");
    TitleStatus ts = title_detect(&g.title);
    if (ts == TITLE_ERR_SERVICE) {
        printf("E-01: Kein Zugriff auf die Titelliste (%08lX).\n", (unsigned long)g.title.error);
        printf("Ist Luma3DS aktiv?\n");
        return;
    }
    if (ts == TITLE_NOT_INSTALLED) {
        printf("E-02: StreetPass Mii Plaza nicht gefunden.\n");
        return;
    }
    printf("Mii Plaza %s gefunden.\n", g.title.title->code);
    if (fs_archives_init(&g.fs, g.title.title) != STORE_OK) {
        printf("SD-Karte nicht nutzbar (%08lX).\n", (unsigned long)g.fs.last_error);
        return;
    }
    fs_archives_storage(&g.fs, &g.storage);
    g.fs_ok = true;

    check_recovery();
    reload();
    if (!g.data_ok)
        printf("E-01: Spielstand nicht lesbar (%08lX).\n", (unsigned long)g.fs.last_error);
    if (g.view != VIEW_RECOVERY)
        ensure_original();
    printf("\nMenue auf dem unteren Bildschirm.\n");
}

int main(void)
{
    gfxInitDefault();
    consoleInit(GFX_TOP, &g.top);
    consoleInit(GFX_BOTTOM, &g.bottom);
    g.meet = malloc(MEET_SAVE_SIZE);
    if (g.meet == NULL) {
        on_top();
        printf("Kein Speicher.\n");
    } else {
        boot();
    }
    draw_top_view();
    draw_bottom();

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();
        if (down & KEY_START)
            break;
        if (g.meet != NULL && handle(down)) {
            draw_top_view();
            draw_bottom();
        }
        wait_for_draw();
    }

    model_free(&s_model);
    if (g.fs_ok)
        fs_archives_exit(&g.fs);
    free(g.meet);
    gfxExit();
    return 0;
}
