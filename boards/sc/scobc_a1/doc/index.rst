.. zephyr:board:: scobc_a1

Overview
********

`Space Cubics`_ OBC Module A1 (SC-OBC Module A1) is a single board computer for spacecraft,
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

.. zephyr:board-supported-hw::

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
   :board: scobc_a1
   :goals: flash

After flashing, you should see message similar to the following in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.1.0-4619-gd571a59b0a43 ***
   Hello World! scobc_a1/designstart_fpga_cortex_m3

Note, however, that the application was not persisted in flash memory by the
above steps. It was merely written to internal RAM in the FPGA.

You can flash the board using an external debug adapter such as CMSIS-DAP, FT2232D, or FT232R. By
default, the SC-OBC Module A1 uses a CMSIS-DAP interface (e.g., Raspberry Pi Pico Debug Probe).
However, if you're using an adapter such as FT2232D or FT232R, specify the interface via the
``OPENOCD_INTERFACE`` environment variable before running ``west flash``.

For example:

.. code-block:: console

   $ export OPENOCD_INTERFACE=FT2232D
   $ west flash

   $ export OPENOCD_INTERFACE=FT232R
   $ west flash

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: scobc_a1
   :goals: debug

Step through the application in your debugger, and you should see a message
similar to the following in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.1.0-4619-gd571a59b0a43 ***
   Hello World! scobc_a1/designstart_fpga_cortex_m3

References
**********

.. target-notes::

.. _Space Cubics:
   https://spacecubics.com/
