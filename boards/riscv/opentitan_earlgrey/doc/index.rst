.. _opentitan_earlgrey:

OpenTitan Earl Grey
###################

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
- 2 x SPI
- Various security peripherals

Detailed specification is on the `OpenTitan Earl Grey Chip Datasheet`_.

Supported Features
==================

The ``opentitan_earlgrey`` board configuration is designed and tested to run on
the Earl Grey chip simulated in Verilator, a cycle-accurate HDL simulation tool.

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| Timer     | on-chip    | RISC-V Machine Timer                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling                 |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported on Zephyr porting.

Programming and Debugging
*************************

First, build and install Verilator as described in the `OpenTitan Verilator
Setup`_ guide .

Building and Flashing
=====================

Here is an example for building the :ref:`hello_world` application. The
following steps were tested on OpenTitan master branch @ 6a3c2e98.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: opentitan_earlgrey
   :goals: build

The OpenTitan Vchip_sim_tb tool can take the Zephyr .elf as input and place it
in simulated flash. The OpenTitan test ROM will then run in simulation, read
the manifest header from simulated flash, and begin executing Zephyr from the
entry point.

.. code-block:: console

   $OT_HOME/bazel-bin/hw/build.verilator_real/sim-verilator/Vchip_sim_tb --verbose-mem-load \
   -r $OT_HOME/bazel-out/k8-fastbuild-ST-2cc462681f62/bin/sw/device/lib/testing/test_rom/test_rom_sim_verilator.39.scr.vmem \
   --meminit=otp,$OT_HOME/bazel-out/k8-fastbuild/bin/hw/ip/otp_ctrl/data/img_rma.24.vmem \
   --meminit=flash,$ZEPHYR_PATH/build/zephyr/zephyr.elf

UART output:

.. code-block:: console

   I00000 test_rom.c:135] Version: earlgrey_silver_release_v5-9599-g6a3c2e988, Build Date: 2023-01-17 16:02:09
   I00001 test_rom.c:237] Test ROM complete, jumping to flash (addr: 20000384)!
   *** Booting Zephyr OS build zephyr-v3.2.0-3494-gf0729b494b98 ***
   Hello World! opentitan_earlgrey

References
**********

.. target-notes::

.. _OpenTitan Earl Grey Chip Datasheet: https://docs.opentitan.org/hw/top_earlgrey/doc/

.. _OpenTitan GitHub: https://github.com/lowRISC/opentitan

.. _OpenTitan Verilator Setup: https://docs.opentitan.org/doc/getting_started/setup_verilator/
