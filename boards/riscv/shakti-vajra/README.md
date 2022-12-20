# Zephyr RTOS port for SHAKTI

This branch of Zephyr RTOS contains the changes to support 32-bit SHAKTI e-class. The executables for various applications were generated and sucessfully simulated on SHAKTI e-class environment.

## Setup
Ubuntu Packages needed:
```
$ sudo apt-get install --no-install-recommends git cmake ninja-build gperf ccache doxygen dfu-util device-tree-compiler python3-ply python3-pip python3-setuptools python3-wheel xz-utils file make gcc-multilib autoconf automake libtool librsvg2-bin texlive-latex-base texlive-latex-extra latexmk texlive-fonts-recommended
```

Repository:
```
$ git clone https://gitlab.com/shaktiproject/software/zephyr-rtos.git
$ cd zephyr-rtos
$ git checkout -b shakti-zephyr
$ git pull origin shakti-zephyr
$ pip3 install --user -r scripts/requirements.txt
$ unset GNUARMEMB_TOOLCHAIN_PATH
$ export ZEPHYR_BASE=$PWD
$ export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
$ export CROSS_COMPILE=<tool_chain_path>
Eg. for 32-bit e-class
$ export CROSS_COMPILE=$TOOL_DIR/riscv/bin/riscv32-unknown-elf-
$ source zephyr-env.sh
```

## Building sample application

```
$ cd samples/<application_dir>
$ mkdir build 
$ cd build
$ cmake -GNinja -DBOARD=shakti ..
$ ninja 
```
The compiled binary is at ``$ZEPHYR_BASE/samples/<application_dir>/build/zephyr/zephyr.elf``


## Sample applications run on e-class

* hello_world
* synchronization
* philosophers
