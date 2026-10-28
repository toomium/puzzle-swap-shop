#!/bin/sh
# Turns the SVG sources in gfx/src/ into the PNGs and gfx/ui.t3s that
# tex3ds packs into romfs/gfx/ui.t3x during `make`.
#
# Needs Node.js and Playwright with Chromium.
# Run after changing or adding an SVG and commit the PNGs with it.
#
# Usage: tools/export_gfx.sh   (from anywhere)
set -eu
cd "$(dirname "$0")/.."
rm -f gfx/*.png
node tools/export_gfx.mjs
