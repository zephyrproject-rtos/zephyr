#!/bin/sh

#
# Copyright (c) 2024 Schneider Electric.
#
# SPDX-License-Identifier: Apache-2.0
#

# Create a FAT12 disk image to flash in memory
# This tool requires the following to be available on the host system:
#
# - dosfstools
# - mtools
set -e

PROGNAME=$(basename "$0")
OUTPUT=$1
LOGICAL_SECTOR_SIZE=512
DISK_SIZE_KB=128

usage() {
    printf "Usage:\n\t%s output.img input1 [input2] [...]" "$PROGNAME"
}

die() {
     >&2 printf "%s ERROR: " "$PROGNAME"
    # We want die() to be usable exactly like printf
    # shellcheck disable=SC2059
    >&2 printf "%s\n" "$@"
    exit 1 
}

if [ $# -lt 2 ]; then
    usage
    die "Not enough arguments."
fi

shift
INPUTS="$*"

printf "Creating empty '%s' image..." "$(basename "$OUTPUT")"
dd if=/dev/zero of="$OUTPUT" bs=1k count=${DISK_SIZE_KB} status=none || die "dd to $OUTPUT"
printf "done\nCreating FAT partition image..."
mkfs.fat -F12 -S"$LOGICAL_SECTOR_SIZE" "$OUTPUT" >/dev/null || die "mkfs.vfat failed"
printf "done\nCopying input files..."
mcopy -i "$OUTPUT" "$INPUTS" "::/" || die "mcopy $OUTPUT $INPUTS"
printf "done\n"
