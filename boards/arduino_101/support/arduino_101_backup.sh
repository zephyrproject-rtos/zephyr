#!/bin/bash

if [ -z ${ZEPHYR_SDK_INSTALL_DIR} ];
then
    echo "ZEPHYR_SDK_INSTLL_DIR is not set.  Unable to continue"
    exit 1
fi

OPENOCD_ROOT=$ZEPHYR_SDK_INSTALL_DIR/sysroots/i686-pokysdk-linux/usr/
OPENOCD=$OPENOCD_ROOT/bin/openocd
OPENOCD_SCRIPT=$OPENOCD_ROOT/share/openocd/scripts


# dump ROM to one file, and ARC + x86 to another
echo ""
echo "This process can take several minutes and appear as if nothing is"
echo "happening.  Please be patient, as the script will clearly state when"
echo "it is complete."
echo ""
read -p "Press enter to continue..." nothing

$OPENOCD -s ${OPENOCD_SCRIPT} \
         -f ${ZEPHYR_BASE}/boards/arduino_101/support/restore.cfg \
         -c 'init' \
         -c 'targets 1' \
         -c 'reset halt' \
         -c 'dump_image A101_OS.bin 0x40000000 393216' \
         -c 'dump_image A101_BOOT.bin 0xffffe000 8192' \
         -c 'shutdown'

echo "Backup process complete."
