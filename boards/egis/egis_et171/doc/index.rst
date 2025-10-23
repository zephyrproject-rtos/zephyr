.. zephyr:board:: egis_et171

Overview
********

The et171 development board is used for development and verification,
and can be used for application development through interfaces such
as UART, GPIO, SPI, and I2C.

Hardware
********

The platform provides following hardware components:

- 32-bit RISC-V CPU
- 384KB embedded SDRAM
- UART
- I2C
- SPI
- GPIO
- PWM
- DMA
- USB

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The et171 platform has 1 GPIO controller. It providing 17 bits of IO.
It is responsible for pin input/output, pull-up, etc.

The et171 platform has 3 SPI controllers, 2 of which can
additionally support QSPI and XIP mode.

The et171 platform has one USB 2.0 controller that supports up to
USB 2.0 high-speed mode.

Except USB DP/DM, all et171 peripheral I/O devices are mapped to 31
I/O pins through the multiplexer controller.

System Clock
------------

The et171 platform has a multi-stage frequency divider with a maximum
speed of 200 MHz.

Programming and debugging
*************************

.. zephyr:board-supported-runners::

The et171 is compatible with Andes ae350, so you can use Andes ICE
for debugging. For debugging zephyr applications or upload them into
a RAM, you will need to connect ICE from host computer to et171 board
and execute the ICE management software, ICEman, on this host computer.

Connecting Andes ICE (AICE)
===========================

AICE is used for debugging and uploading RAM code to the board.

Building
========

You can build applications in the usual way. Here is an example for
the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: egis_et171
   :goals: build

Flashing
========

Since this is a B2B prototype, there is no publicly available
flashing tool. If you have any development needs, please contact
the contact window of `Egis Technology Inc. <https://www.egistec.com/>`_
.

If you has a debugger (`AICE-MICRO`_) with ICEman (`Andes Development Kit`_)
, set the ``CONFIG_XIP=n`` to enable running the program in RAM mode via
the ICE debugger.

At first, you should run ICEman before load program.

.. code-block:: console

   # Enable execute file permission of ICEman
   $ chmod a+x ./ICEman

   # Running the ICEman server
   $ sudo ./ICEman -Z v5

.. code-block:: console

    # When your ICEman server is running
    # You can load the program into RAM and execute it via gdb
    $ riscv64-zephyr-elf-gdb build/zephyr/zephyr.elf
    (gdb) target remote :1111
    (gdb) monitor reset halt
    (gdb) load
    (gdb) quit

References
**********

.. target-notes::

.. _AICE-MICRO: http://www.andestech.com/en/products-solutions/andeshape-platforms/aice-micro/

.. _Andes Development Kit: https://github.com/andestech/Andes-Development-Kit/releases
