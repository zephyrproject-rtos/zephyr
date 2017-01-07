#!/bin/sh

# Just a place holder for any custimizations.

do_debugserver() {
	echo "Detached GDB server"
}

CMD="$1"
shift

case "${CMD}" in
  debugserver)
    do_debugserver "$@"
    ;;
esac
