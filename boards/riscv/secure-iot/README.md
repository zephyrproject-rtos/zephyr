# Zephyr RTOS port for SECURE-IOT

This branch of Zephyr RTOS contains the changes to support 64-bit SHAKTI c-class. The executables for various applications were generated and sucessfully simulated on SHAKTI c-class environment.

## Setup
Ubuntu Packages needed:
```
$ sudo apt-get install --no-install-recommends git cmake ninja-build gperf ccache doxygen dfu-util device-tree-compiler python3-ply python3-pip python3-setuptools python3-wheel xz-utils file make gcc-multilib autoconf automake libtool librsvg2-bin texlive-latex-base texlive-latex-extra latexmk texlive-fonts-recommended
```

Repository:
```
$ git clone --recursive https://gitlab.com/shaktiproject/software/shakti-tools.git
$ git clone https://github.com/Mindgrove-Technologies/zephyr.git zephyr-mindgrove
$ cd zephyr-mindgrove
$ git checkout -b seciot-1.0.1
$ git pull origin seciot-1.0.1
$ pip3 install --user -r scripts/requirements.txt
$ unset GNUARMEMB_TOOLCHAIN_PATH
$ export ZEPHYR_BASE=$PWD
$ export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
$ export CROSS_COMPILE=<tool_chain_path>

Eg. for 64-bit c-class

$ export CROSS_COMPILE=/path/to/opt/riscv/bin/riscv64-unknown-elf-
$ source zephyr-env.sh
```
## Building sample application using Ninja

```
$ cd samples/<application_dir>
$ mkdir build 
$ cd build
$ cmake -GNinja -DBOARD=secure-iot ..
$ ninja
```

## For Changing the Configuration using Ninja

```
$ ninja menuconfig
$ ninja
```

## Building sample application using Make

```
$ cd samples/<application_dir>
$ mkdir build 
$ cd build
$ cmake -DBOARD=secure-iot ..
$ make
```

## For Changing the Configuration using Ninja

```
$ make menuconfig
$ make
```

The compiled binary is at ``$ZEPHYR_BASE/samples/<application_dir>/build/zephyr/zephyr.elf``
