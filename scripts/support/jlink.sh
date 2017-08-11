#!/bin/sh

# This script is loosly based on a script with same purpose provided
# by RIOT-OS (https://github.com/RIOT-OS/RIOT)

JLINK_GDBSERVER=${JLINK_GDBSERVER:-JLinkGDBServer}
JLINK_IF=${JLINK_IF:-swd}
BIN_NAME=${O}/${KERNEL_BIN_NAME}
ELF_NAME=${O}/${KERNEL_ELF_NAME}
GDB_PORT=${GDB_PORT:-2331}

test_config() {
    if ! which ${JLINK_GDBSERVER} >/dev/null 2>&1; then
        echo "Error: Unable to locate JLink GDB server: ${JLINK_GDBSERVER}"
        exit 1
    fi
}

test_bin() {
    if [ ! -f "${BIN_NAME}" ]; then
        echo "Error: Unable to locate image binary: ${BIN_NAME}"
        exit 1
    fi
}

do_debug() {
    do_debugserver 1 &

    # connect to the GDB server
    ${GDB} ${TUI} ${ELF_NAME} \
	-ex "target remote :${GDB_PORT}" \
	-ex 'monitor halt' \
	-ex 'load' \
	-ex 'monitor reset'
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

    echo "JLink GDB server running on port ${GDB_PORT}"
    ${SETSID} ${JLINK_GDBSERVER} \
	-port ${GDB_PORT} \
	-if ${JLINK_IF} \
	-device ${JLINK_DEVICE} \
	-silent \
	-singlerun
}

CMD="$1"
shift

case "${CMD}" in
  debugserver)
    do_debugserver "$@"
    ;;
  debug)
    do_debug "$@"
    ;;
esac
