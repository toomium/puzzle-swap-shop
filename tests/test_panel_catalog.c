#include <string.h>

#include "greatest.h"
#include "core/panel_catalog.h"

/* Piece counts. */
static const uint8_t expected_sizes[PANEL_COUNT] = {
    15, 15, 15, 15, 15, 15, 15, 24, 24, 40, 40, 24, 24, 24, 40, 40, 40, 40, 24, 15, 15,
    40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 24, 40, 40, 40, 40, 40, 24, 24, 15, 40,
    24, 40, 40, 15, 15, 15, 40, 40, 40, 40, 40, 24, 24, 15, 40, 15, 40, 40, 15, 15, 15,
};

TEST builtin_images_only_for_standard_panels(void)
{
    for (unsigned id = 0; id < PANEL_COUNT; id++) {
        const char *name = panel_catalog_builtin_image(id);
        if (id < PANEL_STANDARD_COUNT) {
            ASSERT(name != NULL);
            ASSERT(strncmp(name, "item_panel_", 11) == 0);
        } else {
            ASSERT(name == NULL);
        }
    }
    ASSERT_STR_EQ("item_panel_Samus.lz77", panel_catalog_builtin_image(0));
    ASSERT_STR_EQ("item_panel_NewMario.lz77", panel_catalog_builtin_image(6));
    PASS();
}

TEST every_panel_matches_documentation(void)
{
    for (unsigned id = 0; id < PANEL_COUNT; id++) {
        const PanelInfo *info = panel_catalog_get(id);
        ASSERT(info != NULL);
        ASSERT_EQ(id, info->id);
        ASSERT_EQ(expected_sizes[id], info->piece_count);
        ASSERT_EQ(id < PANEL_STANDARD_COUNT ? PANEL_STANDARD : PANEL_SPOTPASS, info->kind);
        ASSERT(info->fallback_name != NULL && info->fallback_name[0] != '\0');
    }
    PASS();
}

TEST unknown_panel_is_null(void)
{
    ASSERT_EQ(NULL, panel_catalog_get(PANEL_COUNT));
    ASSERT_EQ(NULL, panel_catalog_get(255));
    PASS();
}

TEST piece_counts_fit_mask(void)
{
    for (unsigned id = 0; id < PANEL_COUNT; id++)
        ASSERT(panel_catalog_get(id)->piece_count <= PANEL_MAX_PIECES);
    PASS();
}

TEST grid_columns(void)
{
    ASSERT_EQ(5, panel_catalog_columns(15));
    ASSERT_EQ(6, panel_catalog_columns(24));
    ASSERT_EQ(8, panel_catalog_columns(40));
    ASSERT_EQ(0, panel_catalog_columns(16));
    for (unsigned id = 0; id < PANEL_COUNT; id++) {
        unsigned count = panel_catalog_get(id)->piece_count;
        ASSERT_EQ(0, count % panel_catalog_columns(count));
    }
    PASS();
}

SUITE(panel_catalog_suite)
{
    RUN_TEST(every_panel_matches_documentation);
    RUN_TEST(unknown_panel_is_null);
    RUN_TEST(piece_counts_fit_mask);
    RUN_TEST(grid_columns);
    RUN_TEST(builtin_images_only_for_standard_panels);
}
