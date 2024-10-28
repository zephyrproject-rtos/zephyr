.. zephyr:board:: neorv32

Overview
********

The NEORV32 is an open-source RISC-V compatible processor system intended as a
ready-to-go auxiliary processor within larger SoC designs or as a stand-alone
customizable microcontroller.

For more information about the NEORV32, see the following websites:

- `The NEORV32 RISC-V Processor GitHub`_
- `The NEORV32 RISC-V Processor Datasheet`_
- `The NEORV32 RISC-V Processor User Guide`_

The currently supported version is 1.8.6.

Supported Features
==================

The ``neorv32`` board configuration can be used a generic definition for NEORV32
based boards. Customisation to fit custom NEORV32 implementations can be done
using :ref:`devicetree overlays <use-dt-overlays>`.

Zephyr currently supports the following hardware features of the NEORV32
Processor (SoC):

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| INTC      | on-chip    | interrupt controller                |
+-----------+------------+-------------------------------------+
| MTIME     | on-chip    | system timer                        |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio, non-interrupt                 |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| TRNG      | on-chip    | entropy                             |
+-----------+------------+-------------------------------------+

The default board configuration for the NEORV32 Processor (SoC) can be found in
the defconfig file: :file:`boards/riscv/neorv32/neorv32_defconfig`.

System Clock
============

The default board configuration assumes a system clock of 100 MHz. The clock
frequency can be overridden by changing the ``clock-frequency`` property of the
``cpu0`` devicetree node.

CPU
===

The default board configuration assumes the NEORV32 CPU implementation has the
following RISC-V ISA extensions enabled:

- C (Compresses Instructions)
- M (Integer Multiplication and Division)
- Zicsr (Control and Status Register (CSR) Instructions)

Internal Instruction Memory
===========================

The default board configuration assumes the NEORV32 SoC implementation has a 64k
byte internal instruction memory (IMEM) for code execution. The size of the
instruction memory can be overridden by changing the ``reg`` property of the
``imem`` devicetree node.

Internal Data Memory
====================

The default board configuration assumes the NEORV32 SoC implementation has a 32k
byte internal data memory (DMEM). The size of the data memory can be overridden
by changing the ``reg`` property of the ``dmem`` devicetree node.

Serial Port
===========

The default configuration assumes the NEORV32 SoC implements UART0 for use as
system console.

.. note::

   The default configuration uses a baud rate of 19200 to match that of the
   standard NEORV32 bootloader. The baudrate can be changed by modifying the
   ``current-speed`` property of the ``uart0`` devicetree node.

True Random-Number Generator
============================

The True Random-Number Generator (TRNG) of the NEORV32 is supported, but
disabled by default. For NEORV32 SoC implementations supporting the TRNG,
support can be enabled by setting the ``status`` property of the ``trng``
devicetree node to ``okay``.

Programming and Debugging
*************************

First, configure the FPGA with the NEORV32 bitstream as described in the NEORV32
user guide.

Next, build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Console
=====================

Use the following settings with your serial terminal of choice (minicom, putty,
etc.):

- Speed: 19200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing via JTAG
=================

Here is an example for building and flashing the :zephyr:code-sample:`hello_world` application
for the NEORV32 via JTAG. Flashing via JTAG requires a NEORV32 SoC
implementation with the On-Chip Debugger (OCD) and bootloader enabled.

.. note::

   If the bootloader is not enabled, the internal instruction memory (IMEM) is
   configured as ROM which cannot be modified via JTAG.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: neorv32
   :goals: flash

The default board configuration uses an :ref:`openocd-debug-host-tools`
configuration similar to the example provided by the NEORV32 project. Other
JTAGs can be used by providing further arguments when building. Here is an
example for using the Flyswatter JTAG:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: neorv32
   :goals: flash
   :gen-args: -DBOARD_RUNNER_ARGS_openocd="--config;interface/ftdi/flyswatter.cfg;--config;neorv32.cfg;--cmd-pre-init;'adapter speed 2000'"

After flashing, you should see message similar to the following in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-vn.n.nn  ***
   Hello World! neorv32

Note, however, that the application was not persisted in flash memory by the
above steps. It was merely written to internal block RAM in the FPGA. It will
revert to the application stored in the block RAM within the FPGA bitstream
the next time the FPGA is configured.

The steps to persist the application within the FPGA bitstream are covered by
the NEORV32 user guide. If the :kconfig:option:`CONFIG_BUILD_OUTPUT_BIN` is enabled and
the NEORV32 ``image_gen`` binary is available, the build system will
automatically generate a :file:`zephyr.vhd` file suitable for initialising the
internal instruction memory of the NEORV32.

In order for the build system to automatically detect the ``image_gen`` binary
it needs to be in the :envvar:`PATH` environment variable. If not, the path
can be passed at build time:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: neorv32
   :goals: build
   :gen-args: -DCMAKE_PROGRAM_PATH=<path/to/neorv32/sw/image_gen/>

Uploading via UART
==================

If the :kconfig:option:`CONFIG_BUILD_OUTPUT_BIN` is enabled and the NEORV32
``image_gen`` binary is available, the build system will automatically generate
a :file:`zephyr_exe.bin` file suitable for uploading to the NEORV32 via the
built-in bootloader as described in the NEORV32 user guide.

Debugging via JTAG
==================

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: neorv32
   :goals: debug

Step through the application in your debugger, and you should see a message
similar to the following in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-vn.n.nn  ***
   Hello World! neorv32

.. _The NEORV32 RISC-V Processor GitHub:
   https://github.com/stnolting/neorv32

.. _The NEORV32 RISC-V Processor Datasheet:
   https://stnolting.github.io/neorv32/

.. _The NEORV32 RISC-V Processor User Guide:
   https://stnolting.github.io/neorv32/ug/
