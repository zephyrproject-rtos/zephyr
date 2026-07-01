#!/bin/bash
set -e

echo "=== 1. Initializing Zephyr Workspace ==="
# The zephyrprojectrtos/ci image already has QEMU, CMake, Python, and the SDK installed.
# We just need to pull in the Zephyr modules for this specific workspace.
west init -l .
west update
west zephyr-export
pip3 install -r scripts/requirements.txt

echo "=== 2. Executing Build for QEMU ==="
# We removed the 'host' toolchain override so Zephyr uses the container's built-in SDK cross-compilers.
# Building the basic blinky sample for an emulated ARM Cortex-M3.
west build -b qemu_cortex_m3 samples/basic/blinky

echo "=== 3. Running QEMU Tests ==="
# Execute Twister targeting the QEMU ARM board. 
# Twister will automatically boot the QEMU VM, run the tests, parse the serial output, and shut it down.
west twister -p qemu_cortex_m3 -T tests/kernel -v
