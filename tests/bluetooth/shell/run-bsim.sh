#!/usr/bin/env bash
# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

set -e
set -u

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
# west build -d build -b nrf52_bsim -S xterm-native-shell $ZEPHYR_BASE/tests/bluetooth/shell
default_image=${ZEPHYR_BASE}/tests/bluetooth/shell/build/zephyr/zephyr.exe

num_devices=$1
image="${2:-"${default_image}"}"

# Cleanup all existing sims
$BSIM_OUT_PATH/components/common/stop_bsim.sh

# Force sim to real-time
pushd $BSIM_OUT_PATH/components/device_handbrake
./bs_device_handbrake -s=shell-sim -r=10 -d=0 &
popd

for dev_id in $(seq 1 ${num_devices}); do
    echo "Start device $dev_id"
    $image -s=shell-sim -d=$dev_id -RealEncryption=1 -rs=$dev_id -attach_uart -wait_uart &
done

# Start the PHY
pushd $BSIM_OUT_PATH/bin
$BSIM_OUT_PATH/bin/bs_2G4_phy_v1 -s=shell-sim -D=$((num_devices+=1))
