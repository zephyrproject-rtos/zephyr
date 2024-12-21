.. zephyr:board:: qemu_arc

Overview
********

This board configuration will use QEMU to emulate set of generic
ARCv2 and ARCv3 hardware platforms.

The following features of ARC ISA cores are currently supported:

* CPU:
  * ARCv2 EM
  * ARCv2 HS3x
  * ARCv3 HS5x
  * ARCv3 HS6x
* Only little-endian configurations
* Full 32 register set
* ARC core free-running timers/counters Timer0 & Timer1
* ARC core interrupt controller with multiple priority levels
* DW UART
* 5 slots for MMIO Virtio devices

Hardware
********
Supported Features
==================

The following hardware features are supported:

+--------------+------------+----------------------+
| Interface    | Controller | Driver/Component     |
+==============+============+======================+
| ARCv2 INTC   | on-chip    | interrupt controller |
+--------------+------------+----------------------+
| DW UART      | on-chip    | serial port          |
+--------------+------------+----------------------+
| ARC TIMER0   | on-chip    | system clock         |
+--------------+------------+----------------------+

The kernel currently does not support other hardware features on this platform.

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 1 MHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
DesignWare UART.

Known Problems or Limitations
==============================

The following platform features are unsupported:

* Memory-protection unit (MPU)
* MMIO Virtio Ethernet

Programming and Debugging
*************************

Use this configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment, for example, with the :zephyr:code-sample:`synchronization` sample
(note you may use ``qemu_arc/qemu_em``, ``qemu_arc/qemu_hs``,  ``qemu_arc/qemu_hs5x`` or
``qemu_arc/qemu_hs6x`` depending on target CPU):

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_arc/qemu_em
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        *** Booting Zephyr OS build zephyr-v2.2.0-2486-g7dbfcf4bab57  ***
        threadA: Hello World from qemu_arc!
        threadB: Hello World from qemu_arc!
        threadA: Hello World from qemu_arc!
        threadB: Hello World from qemu_arc!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.

References
**********

1.`Programmer’s Reference Manual for ARC HS
   <https://www.synopsys.com/dw/doc.php/iip/dwc_arc_hs4xd/latest/doc/ARC_V2_PublicProgrammers_Reference.pdf>`_
