#include "core/version.h"

#define PSS_STR2(x) #x
#define PSS_STR(x)  PSS_STR2(x)

const char *version_string(void)
{
    return PSS_STR(PSS_VERSION_MAJOR) "." PSS_STR(PSS_VERSION_MINOR) "." PSS_STR(PSS_VERSION_PATCH);
}
