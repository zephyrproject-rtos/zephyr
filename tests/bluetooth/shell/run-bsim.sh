#!/usr/bin/env bash
# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

set -eu

# Let's source a few small bsim related utilities
source "${ZEPHYR_BASE}/tests/bsim/sh_common.source"

if [ $# -eq 0 ]; then
    echo "Usage: run.sh <number_of_devices> [exe_image]"
    shift
fi

if [ $# -gt 2 ]; then
    echo "Too many args"
    shift
fi

echo "Starting simulation. Hit Ctrl-C to exit."

# Build it with e.g.
# cd $ZEPHYR_BASE/tests/bluetooth/shell
# west build -d build -b nrf52_bsim $ZEPHYR_BASE/tests/bluetooth/shell -- \
#   -DEXTRA_CONF_FILE=xterm-native-shell.conf -DEXTRA_DTC_OVERLAY_FILE=xterm-native-shell.overlay \
#   -DCONFIG_UART_NATIVE_PTY_0_ON_OWN_PTY=y
default_image=${ZEPHYR_BASE}/tests/bluetooth/shell/build/zephyr/zephyr.exe

num_devices=$1
image="${2:-"${default_image}"}"

# Cleanup all existing sims
$BSIM_OUT_PATH/components/common/stop_bsim.sh shell-sim

#Let's stop the simulation after 1h if the user forgets about it
EXECUTE_TIMEOUT=3600

# As we run bsim executables from arbitrary directories, we help them find the bsim libraries
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+${LD_LIBRARY_PATH}:}${BSIM_OUT_PATH}/lib"

for dev_id in $(seq 1 ${num_devices}); do
    echo "Start device $dev_id"
    Execute "$image" -s=shell-sim -d=$dev_id -RealEncryption=1 -attach_uart -mro=10e3
    # The function Execute just runs the command that follows in the background, but remembers its
    # pid. If Ctrl+C is pressed all pending commands run with Execute will be terminated
    # parameters to the zephyr bsim executable:
    #   -s=shell-sim     : Simulation id
    #   -d=$dev_id       : Which device number in that simulation is this one
    #   -RealEncryption=1: Actually encrypt RADIO traffic
    #   -attach_uart     : Automatically attach to the newly created PTY
    #   -mro=10e3        : Synchronize with the phy every 10ms to be more fluid in interactive use
done

cd "$BSIM_OUT_PATH/bin"

# Force sim to real-time
Execute ./bs_device_handbrake -s=shell-sim -r=1 -d=0 -pp=10e3
# parameters to the handbrake:
#  -r=1       : run at 1x of real time (actual real time)
#  -pp=10e3   : Hold down the simulation every 10ms (so it feels fluid for interactive use)
#  -d=$dev_id : Connect the handbrake as device 0

# Start the PHY
Execute ./bs_2G4_phy_v1 -s=shell-sim -D=$((num_devices+1))

# wait_for_background_jobs will wait for all commands run with Execute and return != 0 if any of
# them returns != 0
wait_for_background_jobs
