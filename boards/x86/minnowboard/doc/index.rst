.. _minnowboard_board:

MinnowBoard Max
###############

Overview
********

The MinnowBoard is an Intel |reg| Atom |trade| processor based board which introduces
Intel |reg| Architecture to the small and low cost embedded market for the developer
and maker community. It has exceptional performance, flexibility, openness and
standards

.. figure:: minnowboard.jpg
   :width: 800px
   :align: center
   :alt: Minnowboard

   Minnowboard (Credit: Intel)


The `MinnowBoard`_ board configuration supports the following:

* HPET

* Advanced Programmed Interrupt Controller (APIC)

* NS16550 UART


Hardware
********
Supported Features
==================

This board configuration supports the following hardware features:

+--------------+------------+-----------------------+
| Interface    | Controller | Driver/Component      |
+==============+============+=======================+
| HPET         | on-chip    | system clock          |
+--------------+------------+-----------------------+
| APIC         | on-chip    | interrupt controller  |
+--------------+------------+-----------------------+
| NS16550      | on-chip    | serial port           |
| UART         |            |                       |
+--------------+------------+-----------------------+

The kernel currently does not support other hardware features on this platform.

HPET System Clock Support
--------------------------

This board uses a system clock frequency of 25 MHz.

.. note::
   The LOAPIC timer may be used instead of the HPET. To do so,
   set SYS_CLOCK_HW_CYCLES_PER_SEC to a custom value that is tied to the host system speed.

Serial Port
-----------

This board uses a single serial communication channel
with a NS16550 serial driver that operates in polling mode.
For an interrupt-driven driver, enable the
:option:`CONFIG_UART_INTERRUPT_DRIVEN` kernel configuration option.

PCI
----

PCI drivers assume that IO regions and IRQs for devices are preconfigured
identically by the firmware on all supported devices.  This configuration is
specified in the Kconfig file for the Intel Atom SoC.  The PCI library supports
dynamically enumerating PCI devices, but that support is disabled by default.

.. note::
   The PCI library does not support 64-bit devices.
   Memory address and size storage only require 32-bit integers.


Known Problems or Limitations
-----------------------------

The following platform features are unsupported:

* Isolated Memory Regions
* Serial port in Direct Memory Access (DMA) mode
* Serial Peripheral Interface (SPI) flash
* General-Purpose Input/Output (GPIO)
* Inter-Integrated Circuit (I2C)
* Ethernet
* Supervisor Mode Execution Protection (SMEP)



Creating a GRUB2 Boot Loader Image from a Linux Host
====================================================

Follow the same steps documented for the :ref:`Galileo board <grub2>`.


Booting Zephyr on the MinnowBoard
=================================

The MinnowBoard by default will be running a 64bit firmware. To boot Zephyr, you
will need to replace the 64bit firmware with the 32bit variant. Please follow
the instructions for flashing and updating the firmware documented at the
`MinnowBoard firmware`_ website.


.. _MinnowBoard: https://minnowboard.org/
.. _MinnowBoard firmware: https://minnowboard.org/tutorials/updating-the-firmware
