#!/bin/bash
set -e

echo "=== 1. Installing Zephyr System Dependencies ==="
apt-get update && apt-get install -y --no-install-recommends \
    git cmake ninja-build gperf ccache dfu-util device-tree-compiler \
    wget python3-dev python3-pip python3-venv xz-utils file make \
    gcc gcc-multilib g++-multilib libsdl2-dev libmagic1

echo "=== 2. Setting up Python Virtual Environment & West ==="
# Zephyr strictly requires a virtual environment
python3 -m venv /build/.venv
source /build/.venv/bin/activate
pip install west

echo "=== 3. Initializing Zephyr Workspace ==="
# Tells west to use the current repository as the manifest
west init -l .
west update
west zephyr-export
pip install -r scripts/requirements.txt

echo "=== 4. Executing Build ==="

# Tell Zephyr to use the container's built-in GCC/G++ compiler
export ZEPHYR_TOOLCHAIN_VARIANT=host

# Building a standard sample application for the native Linux simulator
west build -b native_sim samples/basic/blinky

echo "=== 5. Running Native Tests ==="
# Executes Zephyr's built-in testing framework (Twister)
west twister -p native_sim
