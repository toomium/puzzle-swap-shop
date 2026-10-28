#include <stdio.h>

#include "greatest.h"
#include "core/version.h"

TEST version_matches_components(void)
{
    char expected[32];
    snprintf(expected, sizeof(expected), "%d.%d.%d", PSS_VERSION_MAJOR, PSS_VERSION_MINOR,
             PSS_VERSION_PATCH);
    ASSERT_STR_EQ(expected, version_string());
    PASS();
}

SUITE(version_suite)
{
    RUN_TEST(version_matches_components);
}
