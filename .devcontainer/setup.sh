#!/bin/bash
#
# Copyright (c) 2024 Yong Cong Sin <yongcong.sin@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0

# Configurations
## Zephyr SDK version
SDK_VERSION=0.16.8
## Install Directory
INSTALL_DIR="/opt"
## Python Virtual Env
VENV_DIR="$INSTALL_DIR/.venv"

# The owner of dir created in `devcontainer.json` is `root`,
# change it to $USER:$USER here so that we can modify it.
sudo chown -R $USER:$USER ~/zephyrproject

# Install dependencies
# https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies
sudo apt update -y && sudo apt upgrade -y

TMP_DIR=`mktemp -d`
if [[ ! "$TMP_DIR" || ! -d "$TMP_DIR" ]]; then
  echo "Could not create temp dir"
  exit 1
fi

cd "$TMP_DIR"
wget https://apt.kitware.com/kitware-archive.sh
sudo bash kitware-archive.sh

sudo apt install -y --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget \
  python3-dev python3-pip python3-setuptools python3-tk python3-wheel python3-venv xz-utils file \
  make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1

# Install the Zephyr SDK
# https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-the-zephyr-sdk
wget "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${SDK_VERSION}/zephyr-sdk-${SDK_VERSION}_linux-x86_64.tar.xz"
wget -O - "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${SDK_VERSION}/sha256.sum | shasum --check --ignore-missing"
sudo mkdir -p $INSTALL_DIR
sudo chown -R $USER:$USER $INSTALL_DIR
tar xvf "zephyr-sdk-${SDK_VERSION}_linux-x86_64.tar.xz" -C $INSTALL_DIR
cd "$INSTALL_DIR/zephyr-sdk-${SDK_VERSION}"
./setup.sh -t all -h -c

# Cleanup
rm -rf $TMP_DIR
sudo apt-get clean -y && \
	sudo apt-get autoremove --purge -y && \
	sudo rm -rf /var/lib/apt/lists/*

# Create west config file so that we dont have to `west init`
mkdir -p ~/zephyrproject/.west
echo "[manifest]" >> ~/zephyrproject/.west/config
echo "path = zephyr" >> ~/zephyrproject/.west/config
echo "file = west.yml" >> ~/zephyrproject/.west/config
echo "" >> ~/zephyrproject/.west/config
echo "[zephyr]" >> ~/zephyrproject/.west/config
echo "base = zephyr" >> ~/zephyrproject/.west/config
echo "" >> ~/zephyrproject/.west/config
echo "[build]" >> ~/zephyrproject/.west/config
echo "cmake-args = -DCMAKE_EXPORT_COMPILE_COMMANDS=ON" >> ~/zephyrproject/.west/config

# Get Zephyr and install Python dependencies
# https://docs.zephyrproject.org/latest/develop/getting_started/index.html#get-zephyr-and-install-python-dependencies
python3 -m venv $VENV_DIR
source $VENV_DIR/bin/activate

pip install west
cd ~/zephyrproject
source ~/zephyrproject/zephyr/zephyr-env.sh
west update
west zephyr-export
pip install -r ~/zephyrproject/zephyr/scripts/requirements.txt

west completion bash > $INSTALL_DIR/west-completion.bash
source $INSTALL_DIR/west-completion.bash

echo "source ${INSTALL_DIR}/west-completion.bash" >> ~/.bashrc
echo "source ${VENV_DIR}/bin/activate" >> ~/.bashrc
echo "source ~/zephyrproject/zephyr/zephyr-env.sh" >> ~/.bashrc

cd ~/zephyrproject/zephyr
history -c
