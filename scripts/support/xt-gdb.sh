#!/bin/bash

XT_GDB=$XCC_TOOLS/bin/xt-gdb
ELF_NAME=${O}/${KERNEL_ELF_NAME}

set -e

do_debug() {
    ${XT_GDB} ${ELF_NAME}
}

CMD="$1"
shift

case "${CMD}" in
  debug)
    do_debug "$@"
    ;;
  *)
    echo "${CMD} not supported"
    exit 1
    ;;
esac
