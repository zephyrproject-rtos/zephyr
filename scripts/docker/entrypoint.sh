#!/bin/bash

# Start the tap0 interface for qemu testing
source "$HOME/.bashrc"
LOOP_SOCAT_LOG="${LOOP_SOCAT_LOG:-$HOME/loop-socat.log}"
if [ "$LOOP_SOCAT_LOG" == "stdout" ]; then
  $HOME/net-tools/loop-socat.sh 2>&1 &
else
  $HOME/net-tools/loop-socat.sh > "$LOOP_SOCAT_LOG" 2>&1 &
fi
LOOP_SLIP_TAP_LOG="${LOOP_SLIP_TAP_LOG:-$HOME/loop-slip-tap.log}"
if [ "$LOOP_SLIP_TAP_LOG" == "stdout" ]; then
  sudo -E $HOME/net-tools/loop-slip-tap.sh 2>&1 &
else
  sudo -E $HOME/net-tools/loop-slip-tap.sh > "$LOOP_SLIP_TAP_LOG" 2>&1 &
fi
# Run CMD
exec "$@"
