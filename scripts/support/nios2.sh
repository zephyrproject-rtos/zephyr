#!/bin/bash

set -e

HEX_NAME=${O}/${KERNEL_HEX_NAME}
ELF_NAME=${O}/${KERNEL_ELF_NAME}

# XXX nios2-gdb-server doesn't seem to clean up after itself properly,
# and often will report "Unable to bind (98)" when restarting a session.
# Eventually the kernel cleans things up, but it takes a couple minutes.
# Use a random port each time so this doesn't annoy users. Use netstat to
# confirm that the randomly selected port isn't being used.
GDB_TCP_PORT=
while [ -z "$GDB_TCP_PORT" ]; do
    GDB_TCP_PORT=$(shuf -i1024-49151 -n1)
    netstat -atn | grep "127[.]0[.]0[.]1[:]$GDB_TCP_PORT" || break
    GDB_TCP_PORT=
done

REQUIRED_PROGRAMS="quartus_cpf quartus_pgm nios2-gdb-server nios2-download"


for pgm in ${REQUIRED_PROGRAMS}; do
    type -P $pgm > /dev/null 2>&1 || { echo >&2 "$pgm not found in PATH"; exit 1; }
done

# XXX do_flash() and do_debug() only support cases where the .elf is sent
# over the JTAG and the CPU directly boots from __start. CONFIG_XIP and
# CONFIG_INCLUDE_RESET_VECTOR must be disabled.

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
    do_debugserver 1 &

    # connect to the GDB server
    ${GDB} ${TUI} ${ELF_NAME} -ex "target remote :${GDB_TCP_PORT}"
}

do_debugserver() {
    # Calling with an arg will result in setsid being used, which will prevent
    # Ctrl-C in GDB from killing the server. The server automatically exits
    # when the remote GDB disconnects.
    if [ -n "$1" ]; then
        SETSID=/usr/bin/setsid
    else
        SETSID=
    fi
    echo "Nios II GDB server running on port ${GDB_TCP_PORT}"
    ${SETSID} nios2-gdb-server --tcpport ${GDB_TCP_PORT} --stop --reset-target
}


CMD="$1"
shift

case "${CMD}" in
  flash)
    do_flash
    ;;
  debugserver)
    do_debugserver
    ;;
  debug)
    do_debug
    ;;
esac
