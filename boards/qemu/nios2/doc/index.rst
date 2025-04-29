.. zephyr:board:: qemu_nios2

Overview
********

This board configuration will use QEMU to emulate the Altera MAX 10 platform.

This configuration provides support for an Altera Nios-II CPU and these devices:

* Internal Interrupt Controller
* Altera Avalon Timer
* NS16550 UART

.. note::
   This board configuration makes no claims about its suitability for use
   with an actual ti_lm3s6965 hardware system, or any other hardware system.

Hardware
********
Supported Features
==================

The following hardware features are supported:

+--------------+------------+----------------------+
| Interface    | Controller | Driver/Component     |
+==============+============+======================+
| IIC          | on-chip    | Internal interrupt   |
|              |            | controller           |
+--------------+------------+----------------------+
| NS16550      | on-chip    | serial port          |
| UART         |            |                      |
+--------------+------------+----------------------+
| TIMER        | on-chip    | system clock         |
+--------------+------------+----------------------+

The kernel currently does not support other hardware features on this platform.

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 50 MHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART0.

If SLIP networking is enabled (see below), an additional serial port will be
used for it.

Known Problems or Limitations
==============================

The following platform features are unsupported:

* Memory protection through optional MPU.  However, using a XIP kernel
  effectively provides TEXT/RODATA write protection in ROM.
* Writing to the hardware's flash memory
* Serial port in Direct Memory Access (DMA) mode
* Serial Peripheral Interface (SPI) flash
* General-Purpose Input/Output (GPIO)
* Inter-Integrated Circuit (I2C)
* Ethernet


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Use this configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment, for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_nios2
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        ***** BOOTING ZEPHYR OS v1.8.99 - BUILD: Jun 27 2017 13:09:26 *****
        threadA: Hello World from arm!
        threadB: Hello World from arm!
        threadA: Hello World from arm!
        threadB: Hello World from arm!
        threadA: Hello World from arm!
        threadB: Hello World from arm!
        threadA: Hello World from arm!
        threadB: Hello World from arm!
        threadA: Hello World from arm!
        threadB: Hello World from arm!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.

Networking
==========

The board supports SLIP networking over an emulated serial port
(``CONFIG_NET_SLIP_TAP=y``). The detailed setup is described in
:ref:`networking_with_qemu`.

References
**********

* `CPU Documentation <https://www.altera.com/en_US/pdfs/literature/hb/nios2/n2cpu-nii5v1gen2.pdf>`_
* `Nios II Processor Booting Methods in MAX 10 FPGA Devices <https://www.altera.com/en_US/pdfs/literature/an/an730.pdf>`_
* `Embedded Peripherals IP User Guide <https://www.altera.com/content/dam/altera-www/global/en_US/pdfs/literature/ug/ug_embedded_ip.pdf>`_
* `MAX 10 FPGA Configuration User Guide <https://www.altera.com/content/dam/altera-www/global/en_US/pdfs/literature/hb/max-10/ug_m10_config.pdf>`_
* `MAX 10 FPGA Development Kit User Guide <https://www.altera.com/content/dam/altera-www/global/en_US/pdfs/literature/ug/ug-max10m50-fpga-dev-kit.pdf>`_
* `Nios II Command-Line Tools <https://www.altera.com/content/dam/altera-www/global/en_US/pdfs/literature/hb/nios2/edh_ed51004.pdf>`_
* `Quartus II Scripting Reference Manual <https://www.altera.com/content/dam/altera-www/global/en_US/pdfs/literature/manual/tclscriptrefmnl.pdf>`_


.. _Altera Lite Distribution: http://dl.altera.com/?edition=lite
