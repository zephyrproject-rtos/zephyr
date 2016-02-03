#!/bin/bash

if [ -z ${ZEPHYR_SDK_INSTALL_DIR} ];
then
    echo "ZEPHYR_SDK_INSTLL_DIR is not set.  Unable to continue"
    exit 1
fi

OPENOCD_ROOT=$ZEPHYR_SDK_INSTALL_DIR/sysroots/i686-pokysdk-linux/usr/
OPENOCD=$OPENOCD_ROOT/bin/openocd
OPENOCD_SCRIPT=$OPENOCD_ROOT/share/openocd/scripts

if [ ! -r "A101_OS.bin" ];
then
    echo "Unable to find the A101_OS.bin file."
    exit 1
fi

if [ ! -r "A101_BOOT.bin" ];
then
    echo "Unable to find the A101_BOOT.bin file."
    exit 1
fi


# restore ROM one image, and ARC + x86 from another image
$OPENOCD -s ${OPENOCD_SCRIPT} \
         -f ${ZEPHYR_BASE}/boards/arduino_101/support/restore.cfg \
         -c 'init' \
         -c 'targets' \
         -c 'reset halt' \
         -c "load_image   A101_OS.bin 0x40000000" \
         -c "verify_image A101_OS.bin 0x40000000 bin" \
         -c "load_image   A101_BOOT.bin 0xffffe000" \
         -c "verify_image A101_BOOT.bin 0xffffe000 bin" \
         -c "shutdown"
