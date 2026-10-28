#include "greatest.h"
#include "core/geom.h"

static bool rect_eq(GeomRect a, GeomRect b)
{
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

TEST intersect_overlap(void)
{
    GeomRect r;
    ASSERT(geom_rect_intersect((GeomRect){0, 0, 10, 10}, (GeomRect){5, 6, 10, 10}, &r));
    ASSERT(rect_eq(r, (GeomRect){5, 6, 5, 4}));
    PASS();
}

TEST intersect_disjoint_and_touching(void)
{
    GeomRect r;
    ASSERT_FALSE(geom_rect_intersect((GeomRect){0, 0, 10, 10}, (GeomRect){20, 0, 5, 5}, &r));
    ASSERT(geom_rect_empty(r));
    /* Sharing only an edge is no overlap. */
    ASSERT_FALSE(geom_rect_intersect((GeomRect){0, 0, 10, 10}, (GeomRect){10, 0, 5, 5}, &r));
    ASSERT_FALSE(geom_rect_intersect((GeomRect){0, 0, 0, 10}, (GeomRect){0, 0, 5, 5}, &r));
    PASS();
}

TEST contains_edges(void)
{
    GeomRect r = {10, 20, 30, 40};
    ASSERT(geom_rect_contains(r, 10, 20));
    ASSERT(geom_rect_contains(r, 39, 59));
    ASSERT_FALSE(geom_rect_contains(r, 40, 30));
    ASSERT_FALSE(geom_rect_contains(r, 20, 60));
    ASSERT_FALSE(geom_rect_contains(r, 9, 30));
    PASS();
}

TEST inset_grows_and_shrinks(void)
{
    ASSERT(rect_eq(geom_rect_inset((GeomRect){10, 10, 20, 20}, 4), (GeomRect){6, 6, 28, 28}));
    ASSERT(rect_eq(geom_rect_inset((GeomRect){10, 10, 20, 20}, -2), (GeomRect){12, 12, 16, 16}));
    PASS();
}

TEST scissor_full_screen(void)
{
    GeomScissor s = geom_scissor((GeomRect){0, 0, 400, 240}, 400, 240);
    ASSERT_EQ(0u, s.left);
    ASSERT_EQ(0u, s.top);
    ASSERT_EQ(240u, s.right);
    ASSERT_EQ(400u, s.bottom);
    PASS();
}

TEST scissor_rotated_and_mirrored(void)
{
    /* Bottom screen, 100 x 50 box at (10, 20). */
    GeomScissor s = geom_scissor((GeomRect){10, 20, 100, 50}, 320, 240);
    ASSERT_EQ(240u - 70u, s.left);
    ASSERT_EQ(320u - 110u, s.top);
    ASSERT_EQ(240u - 20u, s.right);
    ASSERT_EQ(320u - 10u, s.bottom);
    PASS();
}

TEST scissor_cut_to_screen(void)
{
    GeomScissor s = geom_scissor((GeomRect){-10, 230, 50, 50}, 320, 240);
    /* Visible part: x 0..40, y 230..240. */
    ASSERT_EQ(0u, s.left);
    ASSERT_EQ(280u, s.top);
    ASSERT_EQ(10u, s.right);
    ASSERT_EQ(320u, s.bottom);

    GeomScissor none = geom_scissor((GeomRect){500, 0, 10, 10}, 400, 240);
    ASSERT_EQ(none.left, none.right);
    PASS();
}

TEST nine_slice_stretches_center(void)
{
    GeomPatch p[GEOM_NINE_SLICE_MAX];
    int n = geom_nine_slice(24, 32, (GeomInsets){8, 8, 8, 10}, (GeomRect){100, 50, 120, 32}, p);
    ASSERT_EQ(9, n);
    /* Top left corner keeps its size. */
    ASSERT(rect_eq(p[0].src, (GeomRect){0, 0, 8, 8}));
    ASSERT(rect_eq(p[0].dst, (GeomRect){100, 50, 8, 8}));
    /* Center: 8 x 14 source to 104 x 14. */
    ASSERT(rect_eq(p[4].src, (GeomRect){8, 8, 8, 14}));
    ASSERT(rect_eq(p[4].dst, (GeomRect){108, 58, 104, 14}));
    /* Bottom right corner sits at the far edge. */
    ASSERT(rect_eq(p[8].src, (GeomRect){16, 22, 8, 10}));
    ASSERT(rect_eq(p[8].dst, (GeomRect){212, 72, 8, 10}));
    PASS();
}

TEST nine_slice_patches_cover_dst(void)
{
    GeomPatch p[GEOM_NINE_SLICE_MAX];
    GeomRect dst = {3, 7, 61, 45};
    int n = geom_nine_slice(20, 20, (GeomInsets){5, 6, 7, 8}, dst, p);
    int area = 0;
    for (int i = 0; i < n; i++) {
        GeomRect r;
        ASSERT(geom_rect_intersect(p[i].dst, dst, &r));
        ASSERT(rect_eq(r, p[i].dst));
        area += p[i].dst.w * p[i].dst.h;
    }
    ASSERT_EQ(dst.w * dst.h, area);
    PASS();
}

TEST nine_slice_skips_empty_center(void)
{
    /* Chip: 20 px high image drawn 20 px high, top and bottom borders 10 each. */
    GeomPatch p[GEOM_NINE_SLICE_MAX];
    int n = geom_nine_slice(24, 20, (GeomInsets){10, 10, 10, 10}, (GeomRect){0, 0, 80, 20}, p);
    ASSERT_EQ(6, n);
    for (int i = 0; i < n; i++)
        ASSERT_EQ(10, p[i].dst.h);
    PASS();
}

TEST nine_slice_shrinks_borders(void)
{
    /* Progress bar 5 px high from a 6 px image with 3 px caps. */
    GeomPatch p[GEOM_NINE_SLICE_MAX];
    int n = geom_nine_slice(8, 6, (GeomInsets){3, 3, 3, 3}, (GeomRect){0, 0, 4, 5}, p);
    int area = 0;
    for (int i = 0; i < n; i++) {
        ASSERT(p[i].dst.w > 0 && p[i].dst.h > 0);
        area += p[i].dst.w * p[i].dst.h;
    }
    ASSERT_EQ(20, area);
    PASS();
}

TEST nine_slice_rejects_empty(void)
{
    GeomPatch p[GEOM_NINE_SLICE_MAX];
    ASSERT_EQ(0, geom_nine_slice(24, 32, (GeomInsets){8, 8, 8, 8}, (GeomRect){0, 0, 0, 30}, p));
    ASSERT_EQ(0, geom_nine_slice(0, 32, (GeomInsets){8, 8, 8, 8}, (GeomRect){0, 0, 10, 30}, p));
    PASS();
}

SUITE(geom_suite)
{
    RUN_TEST(intersect_overlap);
    RUN_TEST(intersect_disjoint_and_touching);
    RUN_TEST(contains_edges);
    RUN_TEST(inset_grows_and_shrinks);
    RUN_TEST(scissor_full_screen);
    RUN_TEST(scissor_rotated_and_mirrored);
    RUN_TEST(scissor_cut_to_screen);
    RUN_TEST(nine_slice_stretches_center);
    RUN_TEST(nine_slice_patches_cover_dst);
    RUN_TEST(nine_slice_skips_empty_center);
    RUN_TEST(nine_slice_shrinks_borders);
    RUN_TEST(nine_slice_rejects_empty);
}
