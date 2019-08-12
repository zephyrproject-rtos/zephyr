#!/bin/bash
#-----------------------------------------------------------------------------
# Copyright 2019 Linaro Limited
# SPDX-License-Identifier: Apache-2.0
#
# Merges a secure TF-M library that enables IPC communications with a
# non-secure Zephyr application.
#
# This files is a workaround since we can't currently insert a real post-build
# step in the Zephyr build system, so the zephyr and TF-M binaries need to
# be manually merged pending changes to the build system.
#-----------------------------------------------------------------------------

# Halt on error
set -e

#ZEPHYRBOARD="v2m_musca"
#TFMBOARD="MUSCA_A"
ZEPHYRBOARD="mps2_an521_nonsecure"
TFMBOARD="AN521"
TFMROOT="../../../ext/tfm/tfm"

# Test for tf-m install folder
if [ -d "${TFMROOT}/build/install" ]
then
        echo "Found 'tfm/build/install' dir."
else
        echo "Missing 'tfm/build/install' dir."
        echo "Run 'west build -b ${ZEPHYRBOARD} ./' before continuing."
        exit 1
fi

# Merge tf-m and zephyr binary images
echo "Merging TF-M secure and Zephyr non-secure binaries into tfm_full.hex."
python3 ${TFMROOT}/bl2/ext/mcuboot/scripts/assemble.py -l \
../../../../build/image_macros_preprocessed.c \
-s ${TFMROOT}/build/install/outputs/AN521/tfm_s.bin \
-n build/zephyr/zephyr.bin -o build/tfm_full.bin

# Sign the merged binary
echo "Signing merged binary into tfm_sign.hex"
python3 ${TFMROOT}/bl2/ext/mcuboot/scripts/imgtool.py sign \
--layout ../../../../platform/ext/target/mps2/an521/partition/flash_layout.h \
-k ${TFMROOT}/bl2/ext/mcuboot/root-rsa-3072.pem --align 1 -v 0.0.0+0 \
-H 0x400 build/tfm_full.bin build/tfm_sign.bin

# Copy mcuboot.bin to build
echo "Copying mcuboot.bin"
cp ${TFMROOT}/build/bl2/ext/mcuboot/mcuboot.bin build/mcuboot.bin
