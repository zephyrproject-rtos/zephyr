.. _elemrv_flask_elemrv_h:

ElemRV-H (Hydrogen)
###################

Overview
********

ElemRV-H is the smallest and most area-optimized design in the ElemRV family, built on the Hydrogen platform. It provides a minimal feature set, targeting applications where resource constraints are the primary concern.

Supported Features
******************

System Clock
============

The system clock for the RISC-V core is set to 50 MHz. This value is specified in the ``cpu0`` devicetree node using the ``clock-frequency`` property.

CPU
===

ElemRV-H integrates a VexiiRiscv RISC-V core with the following ISA extensions:

* Zicsr – Control and Status Register operations
* Zifencei – Instruction-fetch fence

The complete ISA string for this CPU is: ``RV32I_Zicsr_Zifencei``

Hart-Level Interrupt Controller (HLIC)
======================================

Each CPU core is equipped with a Hart-Level Interrupt Controller, configurable through Control and Status Registers (CSRs).

Machine Timer
=============

A RISC-V compliant machine timer is enabled by default.

Serial
======

The UART (Universal Asynchronous Receiver-Transmitter) interface is a configurable serial communication peripheral used for transmitting and receiving data.

By default, ``uart0`` operates at a baud rate of ``115200``, which can be adjusted via the ElemRV device tree.

To evaluate the UART interface, build and run the following sample:

.. zephyr-app-commands::
   :board: elemrv_flask/elemrv_h
   :zephyr-app: samples/hello_world
   :goals: build

Pin Multiplexer
================

The pin multiplexer allows each pin to be assigned to one of multiple peripheral functions. The following table lists all available pin assignments:

===  =========  ================
Pin  Option 0   Option 1
===  =========  ================
0    gpio0_0    pwm0_out_0
1    gpio0_1    pio0_0
2    gpio0_2    pio0_1
3    gpio0_3    pio0_2
4    uart0_tx   gpio0_4
5    uart0_rx   gpio0_5
6    uart0_cts  gpio0_6
7    uart0_rts  gpio0_7
8    gpio0_8    pwm0_comp_0
9    gpio0_9    i2c0_scl
10   gpio0_10   i2c0_sda
11   gpio0_11   i2c0_interrupt_0
===  =========  ================

.. note::
   PWM, PIO, and I2C drivers are not yet supported in Zephyr.

GPIO
====

The GPIO (General-Purpose Input/Output) interface with tri-state pins is a digital interface that
allows each pin to be configured as either an input or an output, with additional control for
setting the direction of data flow.

The Blinky sample provides a simple way to explore how a GPIO can be used to control an external LED.

.. zephyr-app-commands::
   :board: elemrv_flask/elemrv_h
   :zephyr-app: samples/basic/blinky
   :goals: build
