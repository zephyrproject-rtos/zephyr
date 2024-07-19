unset GNUARMEMB_TOOLCHAIN_PATH
export ZEPHYR_BASE=$PWD
export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
export CROSS_COMPILE=/opt/riscv/bin/riscv64-unknown-elf-
source zephyr-env.sh