.. zephyr:board:: opentitan_earlgrey

Overview
********

The OpenTitan Earl Grey chip is a low-power secure microcontroller that is
designed for several use cases requiring hardware security. The `OpenTitan
Github`_ page contains HDL code, utilities, and documentation relevant to the
chip.

Hardware
********

- RV32IMCB RISC-V "Ibex" core
- 128kB main SRAM
- Fixed-frequency and AON timers
- 32 x GPIO
- 4 x UART
- 3 x I2C
- 2 x SPI host
- 1 x SPI device
- Various security peripherals

Detailed specification is on the `OpenTitan Earl Grey Chip Datasheet`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

First, build and install Verilator as described in the `OpenTitan Verilator
Setup`_ guide .

Building and Flashing
=====================

Here is an example for building the :zephyr:code-sample:`hello_world` application. The
following steps were tested on the OpenTitan ``earlgrey_1.0.0`` branch @ ccd5f716a1
with Verilator 5.052. That branch is frozen against Verilator 4.210, so the
Verilator 5 fix from OpenTitan master commit af9262c4c0 (``--no-timing`` for the
AST power-good models) must be cherry-picked on top of it.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: opentitan_earlgrey
   :goals: build

Build the simulation binary, the test ROM and the OTP image in the OpenTitan
repository:

.. code-block:: console

   cd $OT_HOME
   ./bazelisk.sh build //hw:verilator \
     //sw/device/lib/testing/test_rom:test_rom_sim_verilator \
     //hw/ip/otp_ctrl/data:img_rma

The OpenTitan Vchip_sim_tb tool can take the Zephyr .elf as input and place it
in simulated flash bank 0. The OpenTitan test ROM will then run in simulation,
read the manifest header from simulated flash, and begin executing Zephyr from
the entry point. Use ``bazel cquery --output=files <target>`` to find the exact
output paths; they look like this:

.. code-block:: console

   $OT_HOME/bazel-out/k8-fastbuild/bin/hw/build.verilator_real/sim-verilator/Vchip_sim_tb \
   --meminit=rom,$OT_HOME/bazel-out/k8-fastbuild-ST-<hash>/bin/sw/device/lib/testing/test_rom/test_rom_sim_verilator.39.scr.vmem \
   --meminit=otp,$OT_HOME/bazel-out/k8-fastbuild/bin/hw/ip/otp_ctrl/data/img_rma.24.vmem \
   --meminit=flash0,$ZEPHYR_PATH/build/zephyr/zephyr.elf

UART output (also written to ``uart0.log`` in the working directory):

.. code-block:: console

   I00001 test_rom.c:158] kBuildInfo: scm_revision=54697461
   I00002 test_rom.c:230] Test ROM complete, jumping to flash (addr: 20000400)!
   *** Booting Zephyr OS build v4.5.0-663-g3526ef7d2cbc ***
   Hello World! opentitan_earlgrey/opentitan

References
**********

.. target-notes::

.. _OpenTitan Earl Grey Chip Datasheet: https://opentitan.org/book/hw/top_earlgrey/doc/specification.html

.. _OpenTitan GitHub: https://github.com/lowRISC/opentitan

.. _OpenTitan Verilator Setup: https://opentitan.org/book/doc/getting_started/setup_verilator.html
