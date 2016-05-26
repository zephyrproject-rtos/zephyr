#!/bin/bash

set -e

HEX_NAME=${O}/${KERNEL_HEX_NAME}
ELF_NAME=${O}/${KERNEL_ELF_NAME}

GDB_TCP_PORT=1234

REQUIRED_PROGRAMS="quartus_cpf quartus_pgm nios2-gdb-server"


for pgm in ${REQUIRED_PROGRAMS}; do
    type -P $pgm > /dev/null 2>&1 || { echo >&2 "$pgm not found in PATH"; exit 1; }
done

do_flash() {
    if [ -z "${NIOS2_CPU_SOF}" ]; then
        echo "Please set NIOS2_CPU_SOF variable to location of CPU .sof data"
        exit 1
    fi

    ${ZEPHYR_BASE}/scripts/support/quartus-flash.py \
            --sof ${NIOS2_CPU_SOF} \
            --kernel ${HEX_NAME}
}

do_debug() {
    do_debugserver &

    # connect to the GDB server
    ${GDB} ${TUI} -ex "target remote :${GDB_TCP_PORT}" ${ELF_NAME}
}

do_debugserver() {
    nios2-gdb-server --tcpport ${GDB_TCP_PORT}
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
