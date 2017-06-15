#!/bin/sh

# This script is loosly based on a script with same purpose provided
# by RIOT-OS (https://github.com/RIOT-OS/RIOT)

PYOCD_FLASHTOOL=${PYOCD_FLASHTOOL:-pyocd-flashtool}
PYOCD_GDBSERVER=${PYOCD_GDBSERVER:-pyocd-gdbserver}
BIN_NAME=${O}/${KERNEL_BIN_NAME}
ELF_NAME=${O}/${KERNEL_ELF_NAME}
GDB_PORT=${GDB_PORT:-3333}

PYOCD_BOARD_ID_ARG=""
if [ -n "${PYOCD_BOARD_ID}" ]; then
	PYOCD_BOARD_ID_ARG="-b ${PYOCD_BOARD_ID}"
fi

test_config() {
    if ! which ${PYOCD_FLASHTOOL} >/dev/null 2>&1; then
        echo "Error: Unable to locate pyOCD flash tool: ${PYOCD_FLASHTOOL}"
        exit 1
    fi
    if ! which ${PYOCD_GDBSERVER} >/dev/null 2>&1; then
        echo "Error: Unable to locate pyOCD GDB server: ${PYOCD_GDBSERVER}"
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
    echo "Flashing Target Device"
    ${PYOCD_FLASHTOOL} -t ${PYOCD_TARGET} ${PYOCD_BOARD_ID_ARG} ${BIN_NAME}
}


do_debug() {
    do_debugserver 1 &

    # connect to the GDB server
    ${GDB} ${TUI} ${ELF_NAME} \
	-ex "target remote :${GDB_PORT}" \
	-ex 'load' \
	-ex 'monitor reset halt'
}

do_debugserver() {
    test_config

    # Calling with an arg will result in setsid being used, which will prevent
    # Ctrl-C in GDB from killing the server. The server automatically exits
    # when the remote GDB disconnects.
    if [ -n "$1" ]; then
        SETSID=/usr/bin/setsid
    else
        SETSID=
    fi
    echo "pyOCD GDB server running on port ${GDB_PORT}"
    ${SETSID} ${PYOCD_GDBSERVER} -p ${GDB_PORT} -t ${PYOCD_TARGET} ${PYOCD_BOARD_ID_ARG}
}

CMD="$1"
shift

case "${CMD}" in
  flash)
    do_flash "$@"
    ;;
  debugserver)
    do_debugserver "$@"
    ;;
  debug)
    do_debug "$@"
    ;;
esac
