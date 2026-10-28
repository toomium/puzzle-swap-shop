#include <stdlib.h>
#include <string.h>

#include "greatest.h"
#include "fake_storage.h"
#include "fixture.h"
#include "app/model.h"
#include "app/pnel_load.h"
#include "core/cgfx.h"
#include "core/pnel_index.h"
#include "core/rare_pieces.h"

/* CFG_Language numbers */
enum { LANG_JP = 0, LANG_EN = 1, LANG_DE = 3 };

static uint8_t *s_pnel, *s_meet, *s_coin;
static size_t s_pnel_len, s_meet_len, s_coin_len;
static FakeStorage s_fake;
static PnelSet s_set;

static void setup(void *arg)
{
    (void)arg;
    s_pnel = fixture_load("pnel_spotpass.dat", &s_pnel_len);
    s_meet = fixture_load("meet_sample.dat", &s_meet_len);
    s_coin = fixture_load("gamecoin_120.dat", &s_coin_len);
    fake_storage_init(&s_fake, s_meet, s_meet_len, s_coin, s_coin_len);
}

static void teardown(void *arg)
{
    (void)arg;
    fake_storage_free(&s_fake);
    fixture_free(s_pnel);
    fixture_free(s_meet);
    fixture_free(s_coin);
}

TEST load_finds_files_by_magic_not_by_name(void)
{
    uint8_t other[PNEL_HEADER_SIZE + PNEL_ENTRY_SIZE] = {0}; /* right size, no magic */
    fake_put_boss(&s_fake, "/readme.bin", other, sizeof(other));
    fake_put_boss(&s_fake, "/sub/anything.xyz", s_pnel, s_pnel_len);
    fake_put_boss(&s_fake, "/tiny", "PNEL", 4);

    ASSERT_EQ(PNEL_LOAD_OK, pnel_load(&s_fake.storage, LANG_DE, &s_set));
    ASSERT_EQ(1u, s_set.files);
    ASSERT_STR_EQ("/sub/anything.xyz", s_set.path[0]);
    ASSERT_EQ(2u, s_set.candidates); /* "tiny" is too small to be looked at */
    ASSERT_EQ(3u, s_set.index.accepted);
    ASSERT_EQ(2u, s_set.index.ignored); /* wrong piece count, standard panel */
    PASS();
}

/* The real file is 26 MB and wrapped: the PNEL data starts somewhere inside. */
TEST load_finds_wrapped_pnel_and_reads_its_pictures(void)
{
    enum { PREFIX = 70000 + 3 }; /* more than one scan chunk, not aligned */
    size_t len = PREFIX + s_pnel_len + 100;
    uint8_t *file = calloc(1, len);
    memcpy(file, "PNELxxxx", 8);     /* a false hit that is no valid header */
    memcpy(file + 65534, "PNEL", 4); /* a hit that straddles two chunks, also invalid */
    memcpy(file + PREFIX, s_pnel, s_pnel_len);
    fake_put_boss(&s_fake, "/wrapped", file, len);
    free(file);

    ASSERT_EQ(PNEL_LOAD_OK, pnel_load(&s_fake.storage, LANG_DE, &s_set));
    ASSERT_EQ(1u, s_set.scanned);
    ASSERT_EQ((uint64_t)PREFIX, s_set.base[0]);
    ASSERT_EQ(3u, s_set.index.accepted);
    ASSERT_STR_EQ("Sommerwiese", s_set.index.panel[7].name);

    uint8_t buf[4096];
    size_t n = 0;
    CgfxTexture t;
    ASSERT_EQ(STORE_OK, pnel_read_image(&s_fake.storage, &s_set, 7, buf, sizeof(buf), &n));
    ASSERT_EQ(CGFX_OK, cgfx_find_texture(buf, n, &t));
    PASS();
}

TEST load_ignores_files_without_valid_pnel(void)
{
    uint8_t junk[200000];
    memset(junk, 'P', sizeof(junk));
    memcpy(junk + 100, "PNEL", 4);
    fake_put_boss(&s_fake, "/junk", junk, sizeof(junk));
    ASSERT_EQ(PNEL_LOAD_NO_FILE, pnel_load(&s_fake.storage, LANG_DE, &s_set));
    ASSERT_EQ(1u, s_set.scanned);
    PASS();
}

TEST load_reports_missing_pieces_of_the_picture(void)
{
    ASSERT_EQ(PNEL_LOAD_NO_FILE, pnel_load(&s_fake.storage, LANG_DE, &s_set));
    ASSERT_EQ(0u, s_set.index.accepted);
    PASS();
}

TEST load_without_archive(void)
{
    s_fake.storage.boss_list = NULL;
    ASSERT_EQ(PNEL_LOAD_NO_ARCHIVE, pnel_load(&s_fake.storage, LANG_DE, &s_set));
    PASS();
}

TEST names_in_console_language_with_english_fallback(void)
{
    fake_put_boss(&s_fake, "/p.dat", s_pnel, s_pnel_len);
    ASSERT_EQ(PNEL_LOAD_OK, pnel_load(&s_fake.storage, LANG_DE, &s_set));
    ASSERT_STR_EQ("Sommerwiese", s_set.index.panel[7].name);
    ASSERT_STR_EQ("Lighthouse", s_set.index.panel[9].name); /* only English in the file */
    ASSERT_STR_EQ("Rhythmus", s_set.index.panel[11].name);

    ASSERT_EQ(PNEL_LOAD_OK, pnel_load(&s_fake.storage, LANG_EN, &s_set));
    ASSERT_STR_EQ("Summer Meadow", s_set.index.panel[7].name);
    ASSERT_STR_EQ("", s_set.index.panel[11].name); /* no English name in the file */

    ASSERT_EQ(PNEL_LOAD_OK, pnel_load(&s_fake.storage, LANG_JP, &s_set));
    ASSERT_STR_EQ("Summer Meadow", s_set.index.panel[7].name);
    PASS();
}

TEST ignores_entries_that_do_not_fit_the_catalog(void)
{
    fake_put_boss(&s_fake, "/p.dat", s_pnel, s_pnel_len);
    pnel_load(&s_fake.storage, LANG_EN, &s_set);
    ASSERT(!s_set.index.panel[8].present); /* 40 pieces in the file, 24 in the catalog */
    ASSERT(!s_set.index.panel[3].present); /* standard panels come from the game */
    ASSERT(s_set.index.panel[7].present);
    ASSERT_EQ(5, s_set.index.panel[9].pink);
    PASS();
}

TEST presence_rule(void)
{
    PnelIndex idx;
    pnel_index_init(&idx);
    ASSERT(pnel_index_panel_present(NULL, 0, 0));  /* standard: always */
    ASSERT(!pnel_index_panel_present(NULL, 7, 0)); /* SpotPass without PNEL: only with pieces */
    ASSERT(pnel_index_panel_present(NULL, 7, 1));
    ASSERT(!pnel_index_panel_present(&idx, 7, 0));
    ASSERT(!pnel_index_panel_present(&idx, PANEL_COUNT, 1)); /* unknown id */

    PnelEntry e = {.order = 7, .piece_count_id = 1};
    ASSERT(pnel_index_add(&idx, 0, &e, "A", NULL));
    ASSERT(pnel_index_panel_present(&idx, 7, 0));
    ASSERT(!pnel_index_panel_present(&idx, 10, 0));
    ASSERT(pnel_index_panel_present(&idx, 10, 4)); /* the save proves it exists */

    /* A later entry replaces the earlier one. */
    ASSERT(pnel_index_add(&idx, 1, &e, "", "B"));
    ASSERT_STR_EQ("B", idx.panel[7].name);
    ASSERT_EQ(1, idx.panel[7].file);

    PnelEntry bad = {.order = 200, .piece_count_id = 1};
    ASSERT(!pnel_index_add(&idx, 0, &bad, "A", NULL));
    bad.order = 7;
    bad.piece_count_id = 3;
    ASSERT(!pnel_index_add(&idx, 0, &bad, "A", NULL));
    ASSERT_EQ(2u, idx.ignored);
    PASS();
}

TEST image_is_read_from_the_file_and_decodes(void)
{
    fake_put_boss(&s_fake, "/p.dat", s_pnel, s_pnel_len);
    pnel_load(&s_fake.storage, LANG_EN, &s_set);

    uint8_t buf[4096];
    size_t len = 0;
    ASSERT_EQ(STORE_OK, pnel_read_image(&s_fake.storage, &s_set, 7, buf, sizeof(buf), &len));
    CgfxTexture t;
    ASSERT_EQ(CGFX_OK, cgfx_find_texture(buf, len, &t));
    ASSERT_EQ(16u, t.width);

    ASSERT_EQ(STORE_ERR_SIZE, pnel_read_image(&s_fake.storage, &s_set, 7, buf, 10, &len));
    ASSERT_EQ(STORE_ERR_NOT_FOUND,
              pnel_read_image(&s_fake.storage, &s_set, 10, buf, sizeof(buf), &len));
    ASSERT_EQ(STORE_ERR_NOT_FOUND,
              pnel_read_image(&s_fake.storage, &s_set, 99, buf, sizeof(buf), &len));

    /* Panel 9 is listed, but its image block is no CGFX. */
    ASSERT_EQ(STORE_OK, pnel_read_image(&s_fake.storage, &s_set, 9, buf, sizeof(buf), &len));
    ASSERT_EQ(CGFX_ERR_MAGIC, cgfx_find_texture(buf, len, &t));
    PASS();
}

TEST rare_pattern_known_for_40_pieces_only(void)
{
    uint64_t mask = 123;
    ASSERT(rare_mask_from_pnel(0, 24, &mask)); /* flag 00: none */
    ASSERT_EQ(0u, mask);
    ASSERT(rare_mask_from_pnel(1, 40, &mask));
    ASSERT_EQ(8, __builtin_popcountll(mask));
    ASSERT(mask & (1ull << 11) && mask & (1ull << 28));
    ASSERT(!(mask & (1ull << 0)));
    mask = 123;
    ASSERT(!rare_mask_from_pnel(1, 24, &mask)); /* pattern of 24 pieces unknown */
    ASSERT_EQ(0u, mask);
    mask = 123;
    ASSERT(!rare_mask_from_pnel(5, 40, &mask)); /* unknown flag value */
    ASSERT_EQ(0u, mask);
    PASS();
}

TEST model_uses_pnel(void)
{
    ModelEnv env = {&s_fake.storage, mii_plaza_title(PLAZA_EUR), NULL, "0.1.0", LANG_DE};
    Model m;
    fake_put_boss(&s_fake, "/p.dat", s_pnel, s_pnel_len);
    model_load(&m, &env);
    ASSERT_EQ(PNEL_LOAD_OK, m.pnel_status);

    ASSERT_STR_EQ("Sommerwiese", model_panel(&m, 7)->name);
    ASSERT_STR_EQ("Star Fox 64 3D", model_panel(&m, 8)->name); /* not in the file: catalog name */
    ASSERT(model_panel(&m, 11)->available);                    /* listed, no piece yet */
    ASSERT_EQ(0u, model_panel(&m, 11)->owned);
    ASSERT(!model_panel(&m, 12)->available); /* neither listed nor owned */
    ASSERT_EQ(0u, model_panel(&m, 9)->rare); /* format unknown: nothing marked */

    /* Listed panels can be bought from; the shop sees them as available. */
    PurchasePlan plan;
    ASSERT_EQ(SHOP_OK, model_plan(&m, 11, 1, &plan));
    ASSERT_EQ(SHOP_ERR_UNAVAILABLE, model_plan(&m, 12, 1, &plan));

    uint8_t buf[4096];
    size_t len;
    CgfxTexture t;
    ASSERT_EQ(STORE_OK, model_panel_image(&m, 7, buf, sizeof(buf), &len));
    ASSERT_EQ(CGFX_OK, cgfx_find_texture(buf, len, &t));
    /* Panel 11 is stored LZ11 packed and comes out unpacked. */
    ASSERT_EQ(STORE_OK, model_panel_image(&m, 11, buf, sizeof(buf), &len));
    ASSERT_EQ(CGFX_OK, cgfx_find_texture(buf, len, &t));
    ASSERT_EQ(STORE_ERR_SIZE, model_panel_image(&m, 11, buf, 100, &len));
    ASSERT_EQ(STORE_ERR_SIZE, model_panel_image(&m, 7, buf, 100, &len));
    model_free(&m);
    PASS();
}

TEST model_without_pnel_keeps_old_rules(void)
{
    ModelEnv env = {&s_fake.storage, mii_plaza_title(PLAZA_EUR), NULL, "0.1.0", LANG_DE};
    Model m;
    model_load(&m, &env);
    ASSERT_EQ(PNEL_LOAD_NO_FILE, m.pnel_status);
    ASSERT_STR_EQ("Star Fox 64 3D", model_panel(&m, 8)->name);
    ASSERT(model_panel(&m, 7)->available); /* owned pieces */
    ASSERT(!model_panel(&m, 11)->available);
    ASSERT_EQ(STORE_ERR_NOT_FOUND, model_panel_image(&m, 7, NULL, 0, &(size_t){0}));
    model_free(&m);
    PASS();
}

TEST builtin_panel_picture_comes_from_the_sd_card(void)
{
    ModelEnv env = {&s_fake.storage, mii_plaza_title(PLAZA_EUR), NULL, "0.1.0", LANG_DE};
    Model m;
    model_load(&m, &env);
    uint8_t buf[4096];
    size_t len, cgfx_len;
    CgfxTexture t;
    ASSERT(model_panel_has_image(&m, 5));
    ASSERT(!model_panel_has_image(&m, 12));
    ASSERT_EQ(7u, model_builtin_pictures_missing(&m));
    /* Not copied yet: no picture, the placeholder stays. */
    ASSERT_EQ(STORE_ERR_NOT_FOUND, model_panel_image(&m, 5, buf, sizeof(buf), &len));

    uint8_t *cgfx = fixture_load("sample_rgb565_16x8.cgfx", &cgfx_len);
    ASSERT(fake_mkdir(&s_fake, "panels"));
    ASSERT(fake_put(&s_fake, "panels/item_panel_Pikmin.lz77", cgfx, cgfx_len));
    ASSERT_EQ(6u, model_builtin_pictures_missing(&m));
    ASSERT_EQ(STORE_OK, model_panel_image(&m, 5, buf, sizeof(buf), &len));
    ASSERT_EQ(CGFX_OK, cgfx_find_texture(buf, len, &t));
    ASSERT_EQ(STORE_ERR_SIZE, model_panel_image(&m, 5, buf, 100, &len));
    /* Other panels do not read from that folder. */
    ASSERT_EQ(STORE_ERR_NOT_FOUND, model_panel_image(&m, 12, buf, sizeof(buf), &len));
    free(cgfx);
    model_free(&m);
    PASS();
}

SUITE(pnel_index_suite)
{
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    RUN_TEST(load_finds_files_by_magic_not_by_name);
    RUN_TEST(load_finds_wrapped_pnel_and_reads_its_pictures);
    RUN_TEST(load_ignores_files_without_valid_pnel);
    RUN_TEST(load_reports_missing_pieces_of_the_picture);
    RUN_TEST(load_without_archive);
    RUN_TEST(names_in_console_language_with_english_fallback);
    RUN_TEST(ignores_entries_that_do_not_fit_the_catalog);
    RUN_TEST(presence_rule);
    RUN_TEST(image_is_read_from_the_file_and_decodes);
    RUN_TEST(rare_pattern_known_for_40_pieces_only);
    RUN_TEST(model_uses_pnel);
    RUN_TEST(model_without_pnel_keeps_old_rules);
    RUN_TEST(builtin_panel_picture_comes_from_the_sd_card);
}
