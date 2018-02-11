.. _zedboard_pulpino:

Zedboard Pulpino
################

Overview
********

By default this board will be built without accounting for
pulpino-specific ISA. To account for them, edit
zedboard_pulpino_defconfig and set CONFIG_RISCV_GENERIC_TOOLCHAIN=n.

However, in this case, a pulpino-specific toolchain should be
use, by setting the following env variables, prior to compiling
an application.

To setup the build system with the correct toolchain use::

        export RISCV32_TOOLCHAIN_PATH=~/path/to/pulpino/toolchain
        export ZEPHYR_TOOLCHAIN_VARIANT=riscv32
