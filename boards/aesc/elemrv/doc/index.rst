.. zephyr:board:: elemrv

Overview
********

ElemRV-N is an end-to-end open-source RISC-V microcontroller designed using SpinalHDL.

Version 0.2 of ElemRV-N was successfully fabricated using `IHP's Open PDK`_, a 130nm open semiconductor process, with support from `FMD-QNC`_.

For more details, refer to the official `GitHub Project`_.

.. note::
   The currently supported silicon version is ElemRV-N 0.2.

Supported Features
******************

.. zephyr:board-supported-hw::

System Clock
============

The system clock for the RISC-V core is set to 20 MHz. This value is specified in the ``cpu0`` devicetree node using the ``clock-frequency`` property.

CPU
===

ElemRV-N integrates a VexRiscv RISC-V core featuring a 5-stage pipeline and the following ISA extensions:

* M (Integer Multiply/Divide)
* C (Compressed Instructions)

It also includes the following general-purpose ``Z`` extensions:

* Zicntr – Base Counter and Timer extensions
* Zicsr – Control and Status Register operations
* Zifencei – Instruction-fetch fence

The complete ISA string for this CPU is: ``RV32IMC_Zicntr_Zicsr_Zifencei``

Hart-Level Interrupt Controller (HLIC)
======================================

Each CPU core is equipped with a Hart-Level Interrupt Controller, configurable through Control and Status Registers (CSRs).

Machine Timer
=============

A RISC-V compliant machine timer is enabled by default.

Serial
======

The UART (Universal Asynchronous Receiver-Transmitter) interface is a configurable serial communication peripheral used for transmitting and receiving data.

By default, ``uart0`` operates at a baud rate of ``115200``, which can be adjusted via the elemrv device tree.

To evaluate the UART interface, build and run the following sample:

.. zephyr-app-commands::
   :board: elemrv/elemrv_n
   :zephyr-app: samples/hello_world
   :goals: build

GPIO
====

The GPIO (General-Purpose Input/Output) interface with tri-state pins is a digital interface that
allows each pin to be configured as either an input or an output, with additional control for
setting the direction of data flow.

``gpio`` consists of a total of 12 individual pins.

The Blinky sample provides a simple way to explore how a GPIO can be used to control an external LED.

.. zephyr-app-commands::
   :board: elemrv/elemrv_n
   :zephyr-app: samples/basic/blinky
   :goals: build

.. _GitHub Project:
   https://github.com/aesc-silicon/elemrv

.. _IHP's Open PDK:
   https://github.com/IHP-GmbH/IHP-Open-PDK

.. _FMD-QNC:
   https://www.elektronikforschung.de/projekte/fmd-qnc
