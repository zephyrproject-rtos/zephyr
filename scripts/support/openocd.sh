#!/bin/sh

OPENOCD_CMD="${OPENOCD:-openocd} -s ${OPENOCD_DEFAULT_PATH}"
OPENOCD_CONFIG=${ZEPHYR_BASE}/boards/${BOARD_NAME}/support/openocd.cfg
BIN_NAME=${O}/${KERNEL_BIN_NAME}

test_config() {
    if [ ! -f "${OPENOCD_CONFIG}" ]; then
        echo "Error: Unable to locate OpenOCD configuration file: ${OPENOCD_CONFIG}"
        exit 1
    fi
    if [ ! -f "${OPENOCD}" ]; then
        echo "Error: Unable to locate OpenOCD executable: ${OPENOCD}"
        exit 1
    fi
}

test_bin() {
    if [ ! -f "${BIN_NAME}" ]; then
        echo "Error: Unable to locate image binary: ${BIN_NAME}"
        exit 1
    fi
}

do_flash() {
    test_config
    test_bin

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
}


CMD="$1"
shift

case "${CMD}" in
  flash)
    echo "Flashing Target Device"
    do_flash "$@"
    ;;
esac
