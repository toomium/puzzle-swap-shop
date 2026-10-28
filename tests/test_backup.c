#include <stdlib.h>
#include <string.h>

#include "greatest.h"
#include "fake_storage.h"
#include "fixture.h"
#include "core/gamecoin.h"
#include "core/meet_save.h"
#include "platform/backup.h"

static uint8_t *meet;
static uint8_t *coin;
static size_t meet_len, coin_len;
static FakeStorage fake;

static void setup(void *arg)
{
    (void)arg;
    meet = fixture_load("meet_sample.dat", &meet_len);
    coin = fixture_load("gamecoin_120.dat", &coin_len);
    fake_storage_init(&fake, meet, meet_len, coin, coin_len);
}

static void teardown(void *arg)
{
    (void)arg;
    fake_storage_free(&fake);
    fixture_free(meet);
    fixture_free(coin);
}

static Manifest meta_at(uint8_t second, BackupReason reason)
{
    Manifest m;
    memset(&m, 0, sizeof(m));
    strcpy(m.app_version, "0.1.0");
    m.created = (Timestamp){2026, 10, 26, 19, 24, second};
    m.reason = reason;
    m.title_id = 0x0004001000022800ull;
    return m;
}

static BackupResult make(const char *name, uint8_t second)
{
    Manifest m = meta_at(second, BACKUP_REASON_PURCHASE);
    return backup_create(&fake.storage, name, &m, meet, meet_len, coin, coin_len, NULL);
}

TEST create_list_load(void)
{
    ASSERT(meet != NULL && coin != NULL);
    Manifest m = meta_at(5, BACKUP_REASON_ORIGINAL), written, loaded;
    ASSERT_EQ(BACKUP_OK, backup_create(&fake.storage, BACKUP_ORIGINAL, &m, meet, meet_len, coin,
                                       coin_len, &written));
    ASSERT_EQ(MEET_SAVE_SIZE, written.meet_size);
    ASSERT_EQ(GAMECOIN_SIZE, written.coin_size);
    ASSERT(fake_find(&fake, "backups/000-original/meet.dat") != NULL);
    ASSERT(fake_find(&fake, "backups/000-original/gamecoin.dat") != NULL);
    ASSERT(fake_find(&fake, "backups/000-original/manifest.txt") != NULL);

    uint8_t *meet2 = malloc(MEET_SAVE_SIZE);
    uint8_t coin2[GAMECOIN_SIZE];
    size_t l1, l2;
    ASSERT_EQ(BACKUP_OK, backup_load(&fake.storage, BACKUP_ORIGINAL, &loaded, meet2, MEET_SAVE_SIZE,
                                     &l1, coin2, sizeof(coin2), &l2));
    ASSERT_EQ(0, memcmp(meet2, meet, meet_len));
    ASSERT_EQ(0, memcmp(coin2, coin, coin_len));
    ASSERT_EQ(0, memcmp(&written, &loaded, sizeof(loaded)));
    free(meet2);

    BackupInfo list[4];
    ASSERT_EQ(1, backup_list(&fake.storage, list, 4));
    ASSERT_STR_EQ(BACKUP_ORIGINAL, list[0].name);
    ASSERT_EQ(BACKUP_ERR_EXISTS, backup_create(&fake.storage, BACKUP_ORIGINAL, &m, meet, meet_len,
                                               coin, coin_len, NULL));
    PASS();
}

TEST names_get_suffixes(void)
{
    char name[BACKUP_NAME_MAX];
    Timestamp t = {2026, 10, 26, 19, 24, 5};
    ASSERT_EQ(BACKUP_OK, backup_choose_name(&fake.storage, &t, name));
    ASSERT_STR_EQ("20261026-192405", name);
    ASSERT_EQ(BACKUP_OK, make(name, 5));
    ASSERT_EQ(BACKUP_OK, backup_choose_name(&fake.storage, &t, name));
    ASSERT_STR_EQ("20261026-192405-2", name);
    /* An incomplete folder blocks its name, too. */
    ASSERT(fake_mkdir(&fake, "backups/20261026-192405-2"));
    ASSERT_EQ(BACKUP_OK, backup_choose_name(&fake.storage, &t, name));
    ASSERT_STR_EQ("20261026-192405-3", name);
    for (unsigned s = 3; s <= BACKUP_MAX_SUFFIX; s++) {
        char path[64];
        snprintf(path, sizeof(path), "backups/20261026-192405-%u", s);
        ASSERT(fake_mkdir(&fake, path));
    }
    ASSERT_EQ(BACKUP_ERR_EXISTS, backup_choose_name(&fake.storage, &t, name));
    PASS();
}

TEST failed_create_leaves_nothing(void)
{
    /* Count the calls of a good run, then fail each one in turn. */
    ASSERT_EQ(BACKUP_OK, make("20261026-192405", 5));
    unsigned total = fake.calls;
    fake_storage_free(&fake);

    for (unsigned n = 1; n <= total; n++) {
        fake_storage_init(&fake, meet, meet_len, coin, coin_len);
        fake.fail_call = n;
        BackupResult r = make("20261026-192405", 5);
        ASSERT(r != BACKUP_OK);
        fake_storage_reboot(&fake);
        ASSERT_EQ(NULL, fake_find(&fake, "backups/20261026-192405"));
        ASSERT_EQ(0, backup_list(&fake.storage, NULL, 0));
        fake_storage_free(&fake);
    }
    fake_storage_init(&fake, meet, meet_len, coin, coin_len);
    PASS();
}

TEST load_detects_damage(void)
{
    uint8_t *buf = malloc(MEET_SAVE_SIZE);
    uint8_t cbuf[GAMECOIN_SIZE];
    size_t l1, l2;
    Manifest m;
    ASSERT_EQ(BACKUP_OK, make("20261026-192405", 5));

    const FakeEntry *e = fake_find(&fake, "backups/20261026-192405/meet.dat");
    e->data[0x44BDC] ^= 0x01;
    ASSERT_EQ(BACKUP_ERR_DAMAGED, backup_load(&fake.storage, "20261026-192405", &m, buf,
                                              MEET_SAVE_SIZE, &l1, cbuf, sizeof(cbuf), &l2));
    e->data[0x44BDC] ^= 0x01;
    ASSERT_EQ(BACKUP_OK, backup_load(&fake.storage, "20261026-192405", &m, buf, MEET_SAVE_SIZE, &l1,
                                     cbuf, sizeof(cbuf), &l2));

    ASSERT(fake_put(&fake, "backups/20261026-192405/gamecoin.dat", coin, 10));
    ASSERT_EQ(BACKUP_ERR_DAMAGED, backup_load(&fake.storage, "20261026-192405", &m, buf,
                                              MEET_SAVE_SIZE, &l1, cbuf, sizeof(cbuf), &l2));
    ASSERT(fake_put(&fake, "backups/20261026-192405/manifest.txt", "x", 1));
    ASSERT_EQ(BACKUP_ERR_MANIFEST, backup_load(&fake.storage, "20261026-192405", &m, buf,
                                               MEET_SAVE_SIZE, &l1, cbuf, sizeof(cbuf), &l2));
    ASSERT_EQ(BACKUP_ERR_NOT_FOUND, backup_load(&fake.storage, "20261026-192406", &m, buf,
                                                MEET_SAVE_SIZE, &l1, cbuf, sizeof(cbuf), &l2));
    ASSERT_EQ(BACKUP_ERR_NAME, backup_load(&fake.storage, "../x", &m, buf, MEET_SAVE_SIZE, &l1,
                                           cbuf, sizeof(cbuf), &l2));
    free(buf);
    PASS();
}

TEST list_sorts_and_skips_incomplete(void)
{
    BackupInfo list[8];
    ASSERT_EQ(BACKUP_OK, make("20261026-192407", 7));
    ASSERT_EQ(BACKUP_OK, make(BACKUP_ORIGINAL, 1));
    ASSERT_EQ(BACKUP_OK, make("20261026-192409", 9));
    ASSERT(fake_mkdir(&fake, "backups/20261026-192411"));
    ASSERT(fake_mkdir(&fake, "backups/notes"));
    ASSERT(fake_put(&fake, "backups/readme.txt", "hi", 2));

    ASSERT_EQ(3, backup_list(&fake.storage, list, 8));
    ASSERT_STR_EQ("20261026-192409", list[0].name);
    ASSERT_STR_EQ("20261026-192407", list[1].name);
    ASSERT_STR_EQ(BACKUP_ORIGINAL, list[2].name);
    ASSERT_EQ(3, backup_list(&fake.storage, list, 1));
    ASSERT_STR_EQ("20261026-192409", list[0].name);
    PASS();
}

TEST remove_and_prune(void)
{
    BackupInfo list[8];
    char name[BACKUP_NAME_MAX];
    ASSERT_EQ(BACKUP_OK, make(BACKUP_ORIGINAL, 1));
    for (uint8_t s = 10; s < 15; s++) {
        snprintf(name, sizeof(name), "20261026-1924%02u", (unsigned)s);
        ASSERT_EQ(BACKUP_OK, make(name, s));
    }
    ASSERT_EQ(BACKUP_ERR_PROTECTED, backup_remove(&fake.storage, BACKUP_ORIGINAL));
    ASSERT_EQ(BACKUP_OK, backup_remove(&fake.storage, "20261026-192414"));
    ASSERT_EQ(NULL, fake_find(&fake, "backups/20261026-192414"));

    ASSERT_EQ(2, backup_prune(&fake.storage, 2));
    ASSERT_EQ(3, backup_list(&fake.storage, list, 8));
    ASSERT_STR_EQ("20261026-192413", list[0].name);
    ASSERT_STR_EQ("20261026-192412", list[1].name);
    ASSERT_STR_EQ(BACKUP_ORIGINAL, list[2].name);
    ASSERT_EQ(0, backup_prune(&fake.storage, 2));
    PASS();
}

SUITE(backup_suite)
{
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    RUN_TEST(create_list_load);
    RUN_TEST(names_get_suffixes);
    RUN_TEST(failed_create_leaves_nothing);
    RUN_TEST(load_detects_damage);
    RUN_TEST(list_sorts_and_skips_incomplete);
    RUN_TEST(remove_and_prune);
}
