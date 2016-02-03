if [ -z "$ZEPHYR_BASE" ]; then
	echo "Please source zephyr-env.sh first"
	exit 1
fi


OPENOCD_PRE_CMD="-c targets 1"
BOOT_ROM=$ZEPHYR_BASE/boards/arduino_101/support/quark_se_rom.bin
ROM_LOAD_ADDR=0xffffe000
BOARD_NAME=arduino_101
OPENOCD_LOAD_CMD="load_image $BOOT_ROM $ROM_LOAD_ADDR"
OPENOCD_VERIFY_CMD="verify_image $BOOT_ROM $ROM_LOAD_ADDR"
OPENOCD=${ZEPHYR_SDK_INSTALL_DIR}/sysroots/i686-pokysdk-linux/usr/bin/openocd
OPENOCD_DEFAULT_PATH=${ZEPHYR_SDK_INSTALL_DIR}/sysroots/i686-pokysdk-linux/usr/share/openocd/scripts
OPENOCD_CMD="${OPENOCD:-openocd} -s ${OPENOCD_DEFAULT_PATH}"
OPENOCD_CONFIG=${ZEPHYR_BASE}/boards/${BOARD_NAME}/support/openocd.cfg


# flash device with specified image
sh -c  "${OPENOCD_CMD} -f '${OPENOCD_CONFIG}' \
    -c 'init' \
    -c 'targets' \
    ${OPENOCD_PRE_CMD} \
    -c 'reset halt' \
    -c ${OPENOCD_LOAD_CMD} \
    -c 'reset halt' \
    -c ${OPENOCD_VERIFY_CMD} \
    ${OPENOCD_POST_CMD} \
    -c 'reset run' \
    -c 'shutdown'"
echo 'Done flashing'

