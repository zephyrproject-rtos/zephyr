#! /bin/sh
#
# Copyright (c) 2017 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#
#
# Scan for files without some kind of a license header or
# Copyright. Skip some files (as described in the first grep) due to:
#
# - them being trivial
# - them being covered by the blanket Apache 2.0 license at the top
#   level
#
# The second grep scans for strings that mark a license
# header/copyright and skip those. Print the rest to stdout.
#
# Run on top of the git tree
#
tmpdir=$(mktemp -d)
trap "rm -rf $tmpdir" EXIT
# Filter our things we don't need
git ls-files | sort -u > $tmpdir/files-all
echo "I: $(wc -l < $tmpdir/files-all) files total" 1>&2
grep -v \
     `# Kbuild files, describing configuration, default` \
     `# configs and Makefiles` \
     -e Kbuild -e /Kconfig.\* -e Makefile \
     -e _defconfig$ -e /defconfig$ \
     -e prj\\.\*\\.conf -e \.conf$ \
     -e ^kernel/configs/kernel.config$ \
     `# Linker scripts` \
     -e linked\\.ld$ -e \\.ld$ \
     `# Device tree` \
     -e \\.dts$ -e \\.fixup$ -e \\.dtsi$ \
     -e dts/.*\\.yaml$ \
     `# Zephyr Sanity Check configs` \
     -e scripts/sanity_chk/arches/.*\\.ini \
     -e scripts/sanity_chk/.*\\.args \
     -e scripts/sanity_chk/.*\\.csv \
     `# Documentation files` \
     -e \\.rst$ -e README -e readme\\.txt -e TODO \
     -e ^samples/net/wpanusb/wpan-radio-spec.txt$ \
     `# Cross compiler support` \
     -e ^scripts/cross_compiler/.*\\.config$ \
     `# Testcase descriptor` \
     -e testcase.ini -e defaults\\.tc \
     `# Images we can't scan for text...` \
     -e jpg$ -e png$ \
     `# OpenOCD configuration files` \
     -e openocd.cfg \
     `# Zephyr misc configuration stuff` \
     -e ^\\.known-issues/ -e ^\\.git -e ^\\.checkpatch.conf \
     -e ^\\.mailmap/ -e ^\\.shippable.yml -e \\.gitignore \
     `# Nios data format XML` \
     -e \.dpf$ \
     `# List of maintainers` \
     -e MAINTAINERS -e ^\\.mailmap \
     `# scripts/kconfig: described doc/LICENSING.rst` \
     -e ^scripts/kconfig/ \
     `# ext/fs/fat: described in doc/LICENSING.rst` \
     -e ^ext/fs/fat \
     `# ext/hal/cmsis: a license agreement...` \
     -e ^ext/hal/cmsis/CMSIS_END_USER_LICENCE_AGREEMENT.pdf \
     `# ext/hal/nxp: trivial` \
     -e ^ext/hal/nxp/mcux/devices/MKW40Z4/fsl_clock.c$ \
     -e ^ext/hal/nxp/mcux/devices/MKW40Z4/fsl_clock.h$ \
     -e ^ext/hal/nxp/mcux/middleware/wireless/framework_5.3.3/OSAbstraction/Source/fsl_os_abstraction_zephyr.c$ \
     -e ^samples/bluetooth/hci_uart/.*.overlay$ \
     $tmpdir/files-all > $tmpdir/files-before
echo "I: $(wc -l < $tmpdir/files-before) after filtering known issues" 1>&2

for token in \
    SPDX-License-Identifier \
        Copyright \
        License \
        licenseText \
        \([Cc]\);
do
    grep -Lr "$token" $(<$tmpdir/files-before) > $tmpdir/files-after
    echo "I: $(wc -l < $tmpdir/files-before) files before,"\
         "$(wc -l < $tmpdir/files-after) after filtering token '$token'" 1>&2
    mv $tmpdir/files-after $tmpdir/files-before
done
echo "I: $(wc -l < $tmpdir/files-before) files without license information"  1>&2
cat $tmpdir/files-before
