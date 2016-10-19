#!/bin/sh

# This script is loosly based on a script with same purpose provided
# by RIOT-OS (https://github.com/RIOT-OS/RIOT)

OPENOCD_CMD="${OPENOCD:-openocd} -s ${OPENOCD_DEFAULT_PATH}"
OPENOCD_CONFIG=${ZEPHYR_BASE}/boards/${ARCH}/${BOARD_NAME}/support/openocd.cfg
BIN_NAME=${O}/${KERNEL_BIN_NAME}
ELF_NAME=${O}/${KERNEL_ELF_NAME}

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


do_debug() {
    test_config
    test_bin
    # setsid is needed so that Ctrl+C in GDB doesn't kill OpenOCD
    [ -z "${SETSID}" ] && SETSID="$(which setsid)"
    # temporary file that saves OpenOCD pid
    OCD_PIDFILE=$(mktemp -t "openocd_pid.XXXXXXXXXX")
    # cleanup after script terminates
    trap "cleanup ${OCD_PIDFILE}" EXIT
    # don't trap on Ctrl+C, because GDB keeps running
    trap '' INT
    # start OpenOCD as GDB server
    ${SETSID} sh -c "${OPENOCD_CMD} -f '${OPENOCD_CONFIG}' \
            ${OPENOCD_EXTRA_INIT} \
            -c 'tcl_port ${TCL_PORT:-6333}' \
            -c 'telnet_port ${TELNET_PORT:-4444}' \
            -c 'gdb_port ${GDB_PORT:-3333}' \
            -c 'init' \
            -c 'targets' \
            -c 'halt' \
             & \
            echo \$! > $OCD_PIDFILE" &
    # connect to the GDB server
    ${GDB} ${TUI} -ex "target remote :${GDB_PORT:-3333}" ${ELF_NAME}
    # will be called by trap
    cleanup() {
        OCD_PID="$(cat $OCD_PIDFILE)"
        kill ${OCD_PID} &>/dev/null
        rm -f "$OCD_PIDFILE"
        exit 0
    }
}

do_debugserver() {
    test_config
    sh -c "${OPENOCD_CMD} -f '${OPENOCD_CONFIG}' \
            -c 'init' \
            -c 'targets' \
            -c 'reset halt'"
}

CMD="$1"
shift

case "${CMD}" in
  flash)
    echo "Flashing Target Device"
    do_flash "$@"
    ;;
  debugserver)
    do_debugserver "$@"
    ;;
  debug)
    do_debug "$@"
    ;;
esac
