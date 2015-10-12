#!/bin/sh
# Run socat in a loop. This way we can restart qemu and do not need
# to manually restart socat.

while [ 1 ]; do socat PTY,link=/tmp/slip.dev UNIX-LISTEN:/tmp/slip.sock; done
