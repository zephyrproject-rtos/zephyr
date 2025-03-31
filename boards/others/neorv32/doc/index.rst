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

The currently supported version is NEORV32 v1.11.2.

Supported Board Targets
=======================

The following NEORV32 board targets are supported by Zephyr:

- ``neorv32/neorv32/minimalboot``
- ``neorv32/neorv32/up5kdemo``

Each of these match one of the NEORV32 processor templates provided alongside the NEORV32 RTL.
These board targets can be customized out-of-tree to match custom NEORV32 implementations using
:ref:`board extensions <extend-board>` or :ref:`devicetree overlays <use-dt-overlays>`.

.. zephyr:board-supported-hw::

Supported Features
******************

The following NEORV32 features are supported by Zephyr. These are pre-configured for the supported
board targets, but can be customized to match custom NEORV32 implementations.

System Clock
============

The default board configuration reads the system clock frequency from the NEORV32 SYSINFO module,
which results in a small run-time overhead. If the clock frequency is known at build time, this
behavior can be overridden by setting the ``clock-frequency`` property of the ``cpu0`` devicetree
node.

CPU
===

The SoC configuration assumes the NEORV32 CPU implementation has the following RISC-V ISA extensions
enabled:

- Zicntr (Extension for Base Counters and Timers)
- Zicsr (Control and Status Register (CSR) Instructions, always enabled)
- Zifencei (Instruction-fetch fence, always enabled)

Other supported RISC-V ISA extensions must be enabled via Kconfig on the board level, and the
``riscv,isa`` devicetree property of the ``cpu0`` node must be set accordingly.

Core Local Interruptor
======================

The NEORV32 Core Local Interruptor (CLINT) and its machine timer (MTIMER) are supported but disabled
by default. For NEORV32 SoC implementations supporting these, support can be enabled by setting
the ``status`` properties of the ``clint`` and ``mtimer`` devicetree node to ``okay``.

Internal Instruction Memory
===========================

The internal instruction memory (IMEM) for code execution is supported but disabled by default. For
NEORV32 SoC implementations supporting IMEM, support can be enabled by setting the size via the
``reg`` property of the ``imem`` devicetree node and setting its ``status`` property to ``okay``.

Internal Data Memory
====================

The internal data memory (DMEM) is supported but disabled by default. For NEORV32 SoC
implementations supporting DMEM, support can be enabled by setting the size via the ``reg`` property
of the ``dmem`` devicetree node and setting its ``status`` property to ``okay``.

Serial Port
===========

The NEORV32 serial ports (UART0 and UART1) are supported but disabled by default. For NEORV32 SoC
implementations supporting either of the UARTs, support can be enabled by setting the ``status``
properties of the ``uart0`` and/or ``uart1`` devicetree node to ``okay``.

.. note::
   The board targets provide a console on UART0 with a baud rate of 19200 to match that of the
   standard NEORV32 bootloader. The baudrate can be changed by modifying the ``current-speed``
   property of the ``uart0`` devicetree node.

General Purpose Input/Output
============================

The NEORV32 GPIO port is supported but disabled by default. For NEORV32 SoC implementations
supporting the GPIOs, support can be enabled by setting the ``status`` property of the ``gpio``
devicetree node to ``okay``. The number of supported GPIOs can be set via the ``ngpios`` devicetree
property.

True Random-Number Generator
============================

The True Random-Number Generator (TRNG) of the NEORV32 is supported, but disabled by default. For
NEORV32 SoC implementations supporting the TRNG, support can be enabled by setting the ``status``
property of the ``trng`` devicetree node to ``okay``.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

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
   :board: neorv32/neorv32/<variant>
   :goals: flash

The default board configuration uses an :ref:`openocd-debug-host-tools`
configuration similar to the example provided by the NEORV32 project. Other
JTAGs can be used by providing further arguments when flashing. Here is an
example for using the Flyswatter JTAG @ 2 kHz:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: neorv32/neorv32/<variant>
   :goals: flash
   :flash-args: --config interface/ftdi/flyswatter.cfg --config neorv32.cfg --cmd-pre-init 'adapter speed 2000'

After flashing, you should see message similar to the following in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-vn.n.nn  ***
   Hello World! neorv32/neorv32/<variant>

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
   :board: neorv32/neorv32/<variant>
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
   :board: neorv32/neorv32/<variant>
   :goals: debug

Step through the application in your debugger, and you should see a message
similar to the following in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-vn.n.nn  ***
   Hello World! neorv32/neorv32/<variant>

.. _The NEORV32 RISC-V Processor GitHub:
   https://github.com/stnolting/neorv32

.. _The NEORV32 RISC-V Processor Datasheet:
   https://stnolting.github.io/neorv32/

.. _The NEORV32 RISC-V Processor User Guide:
   https://stnolting.github.io/neorv32/ug/
