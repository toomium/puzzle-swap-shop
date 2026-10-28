#!/bin/sh
# Color values live only in source/ui/theme.c.
#
# Fails if
#   - any source file outside theme.c builds a color (C2D_Color32, C2D_Color32f, RGBA8), or
#   - UI code (source/ui, source/main.c) contains a 6- or 8-digit hex literal or a #RRGGBB string.
# core/ and platform/ are only checked for the color constructors, because they
# legitimately contain hex constants for file formats and result codes.
#
# Usage: tools/check_colors.sh   (from the repository root)
set -eu

THEME=source/ui/theme.c
status=0

ctor=$(grep -rnE '\b(C2D_Color32f?|RGBA8)[[:space:]]*\(' source --include='*.c' --include='*.h' |
    grep -v "^$THEME:" || true)
if [ -n "$ctor" ]; then
    echo "Farbe außerhalb von $THEME gebaut:"
    echo "$ctor"
    status=1
fi

ui_files=$(find source/ui source/main.c -name '*.[ch]' ! -path "$THEME" 2>/dev/null | sort)
if [ -n "$ui_files" ]; then
    # shellcheck disable=SC2086
    hex=$(grep -nHE '0[xX][0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?\b|#[0-9A-Fa-f]{6}\b' $ui_files || true)
    if [ -n "$hex" ]; then
        echo "Farbwert außerhalb von $THEME:"
        echo "$hex"
        status=1
    fi
fi

if [ "$status" -eq 0 ]; then
    echo "Farbwerte: nur in $THEME"
fi
exit "$status"
