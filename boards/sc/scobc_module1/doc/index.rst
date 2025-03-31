.. zephyr:board:: scobc_module1

Overview
********

`Space Cubics`_ OBC module 1 is a single board computer for spacecraft,
especially for 3U CubeSats.  The board is based on Xilinx Artix-7 FPGA and
implements ARM Cortex M3 as the main CPU.

It is designed to survive in the severe space environment, extreme temperature,
vacuum, and space radiation.

As the name suggests, the board form factor is a module and requires a base I/O
board connected at CON1, a board-to-board connector.  This modularity allows
CubeSat designers the freedom to connect and expand the capability required for
their mission.

Hardware
********

Supported Features
==================

The Space Cubics OBC module 1 provides the following hardware features:

+-----------+------------+------------------------------------+
| Interface | Controller | Driver/Component                   |
+===========+============+====================================+
| NVIC      | on-chip    | nested vector interrupt controller |
+-----------+------------+------------------------------------+
| SYSTICK   | on-chip    | systick                            |
+-----------+------------+------------------------------------+
| UART      | on-chip    | serial port-polling;               |
|           |            | serial port-interrupt              |
+-----------+------------+------------------------------------+

The default configuration for the board can be found in the defconfig file:
:file:`boards/arm/scobc_module1/scobc_module1_defconfig`.

Other hardware features are not currently supported by the port.

System Clock
============

The board has two 24 MHz external oscillators connected to the FPGA for
redundancy. The FPGA will select an active oscillator as CPU system clock.  The
selected clock signal is then used by the CMT in the FPGA, and drives the CPU at
48 MHz by default.

Serial Port
===========

The default configuration contains one SC UART IP, which is register compatible
with Xilinx UART Lite for basic TX and RX. This UART is configured as the
default console and is accessible through the CON1 pin 43 and 45 for Rx and Tx,
respectively.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Here is an example for building and flashing the \`hello\_world\`
application for the board:

Here is an example for building and flashing the :zephyr:code-sample:`hello_world` application
for the default design:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: scobc_module1
   :goals: flash

After flashing, you should see message similar to the following in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.7.99  ***
   Hello World! scobc_module1

Note, however, that the application was not persisted in flash memory by the
above steps. It was merely written to internal RAM in the FPGA.

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: scobc_module1
   :goals: debug

Step through the application in your debugger, and you should see a message
similar to the following in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.7.99  ***
   Hello World! scobc_module1

References
**********

.. target-notes::

.. _Space Cubics:
   https://spacecubics.com/
