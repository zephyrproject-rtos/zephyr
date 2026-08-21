#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# Fetch and build the QEMU TCG plugins that zperf_profile.py drives.
#
# The plugins themselves are QEMU's own, and they are deliberately NOT carried
# in the Zephyr tree: they are built against a QEMU header, which makes them
# derivative works of QEMU, and QEMU's terms are not among those this tree
# carries (the LICENSES directory lists what it does). This script contains no
# QEMU code. It downloads the sources, adjusts one constant and compiles them
# into a directory outside the repository. Nothing it produces is redistributed
# with Zephyr, and neither the Zephyr build nor its CI runs it. See
# samples/net/zperf/README-loopback-profiling.rst for the full reasoning.
#
# Two plugins are built:
#
#   libhotblocks.so    counts executed instructions per translation block
#   libstoptrigger.so  stops the guest at an address, used to measure the boot
#                      cost that is subtracted from each transfer profile
#
# Usage:
#   samples/net/zperf/scripts/qemu_plugin_setup.sh [output-directory]
#
# The QEMU tag must match the emulator that will load the plugins, because the
# loader checks the plugin API version. Check yours with:
#
#   qemu-system-i386 --version

set -eu

QEMU_TAG="${QEMU_TAG:-v10.0.2}"
DIR="${1:-$(pwd)/../tools/qemu-plugins}"
RAW="https://gitlab.com/qemu-project/qemu/-/raw/${QEMU_TAG}"

command -v curl >/dev/null 2>&1 || { echo "curl is required" >&2; exit 1; }
command -v gcc >/dev/null 2>&1 || { echo "gcc is required" >&2; exit 1; }
pkg-config --exists glib-2.0 || {
	echo "glib-2.0 development files are required (libglib2.0-dev)" >&2
	exit 1
}

mkdir -p "$DIR/include" "$DIR/src" "$DIR/lib"

echo "Fetching QEMU $QEMU_TAG plugin sources into $DIR"
curl -fsSL -o "$DIR/include/qemu-plugin.h" "$RAW/include/qemu/qemu-plugin.h"
curl -fsSL -o "$DIR/src/hotblocks.c" "$RAW/contrib/plugins/hotblocks.c"
curl -fsSL -o "$DIR/src/stoptrigger.c" "$RAW/contrib/plugins/stoptrigger.c"

# Released QEMU versions print only the 20 hottest blocks: the report limit is
# a hardcoded constant and the plugin parses no argument for it. A profile of a
# whole network stack needs every block, so raise it. Fail loudly if the
# constant ever changes shape rather than silently producing a truncated
# profile that looks plausible.
if grep -q '^static guint64 limit = 20;$' "$DIR/src/hotblocks.c"; then
	sed -i 's/^static guint64 limit = 20;$/static guint64 limit = G_MAXUINT64;/' \
		"$DIR/src/hotblocks.c"
elif grep -q 'limit' "$DIR/src/hotblocks.c"; then
	echo "note: hotblocks.c no longer has the expected output limit constant," >&2
	echo "      so it is built as upstream ships it. zperf_profile.py warns" >&2
	echo "      when a report looks truncated; if it does, raise the limit in" >&2
	echo "      $DIR/src/hotblocks.c by hand and rebuild the plugin." >&2
fi

for plugin in hotblocks stoptrigger; do
	echo "Building lib$plugin.so"
	# The plugin resolves qemu_plugin_* from the QEMU executable at load time,
	# so it links against nothing but glib. Building all of QEMU is not needed.
	gcc -O2 -g -fPIC -shared -Wall \
		-I "$DIR/include" \
		$(pkg-config --cflags glib-2.0) \
		-o "$DIR/lib/lib$plugin.so" "$DIR/src/$plugin.c" \
		$(pkg-config --libs glib-2.0)

	for symbol in qemu_plugin_version qemu_plugin_install; do
		nm -D "$DIR/lib/lib$plugin.so" | grep -q " $symbol\$" || {
			echo "lib$plugin.so does not export $symbol; QEMU would reject it" >&2
			exit 1
		}
	done
done

echo "$QEMU_TAG" > "$DIR/VERSION"

cat <<EOF

Built against QEMU $QEMU_TAG in $DIR/lib:
  libhotblocks.so
  libstoptrigger.so

Profile the zperf loopback run with:
  samples/net/zperf/scripts/zperf_profile.py run --base-dir .. \\
      --plugin $DIR/lib/libhotblocks.so --outdir ../build/zprof
EOF
