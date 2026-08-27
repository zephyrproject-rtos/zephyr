.. zephyr:board:: nexys_ganga

Overview
********

SHAKTI is India's first indigenous open-source RISC-V processor initiative,
developed by RISE group at IIT Madras. Ganga is a 64-bit (RV64IMAC)
SHAKTI SoC, providing CLINT and PLIC interrupt controllers, a machine timer,
and a UART used for the Zephyr console.

This board target runs the Ganga SoC as a softcore on the Digilent Nexys
Video, a Xilinx Artix-7 FPGA board. Applications can be loaded onto the FPGA
over JTAG via the board's on board USB JTAG chip.

The complete hardware sources, together with instructions for building the
bitstream and loading it onto the Nexys Video, are available (see the
references below)

See the following references for more information:

- `SHAKTI Processor Project`_
- `SHAKTI gc2025 sources`_
- `Nexys Video Reference Manual`_

Hardware
********

- Ganga CPU with the RV64IMAC instruction set, running at 40 MHz
- CLINT and PLIC interrupt controllers with a machine timer
- 256 MB of RAM
- UART, used for the Zephyr console at 19200 baud
- JTAG via the on board USB JTAG chip

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The FPGA must be configured with the Ganga bitstream before loading any Zephyr
application, refer to the SHAKTI gc2025 sources for building and loading the
bitstream. Once the bitstream is loaded Zephyr applications are built and
flashed as usual

Configuring a Console
=====================

The board provides a console on UART0. Use the following settings with your
serial terminal of choice:

- Speed: 19200
- Data: 8 bits
- Parity: None
- Stop bits: 1

The baud rate can be changed via the ``current-speed`` property of the
``uart0`` devicetree node.

Flashing
========

``west flash`` is supported via the openocd runner. Here is an example for
the :zephyr:code-sample:`hello_world` application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nexys_ganga
   :goals: flash

After flashing, you should see a message similar to the following in the
terminal:

.. code-block:: console

   *** Booting Zephyr OS build ... ***
   Hello World! nexys_ganga/ganga

The application is loaded into the FPGA block RAM and is not persisted, it
reverts the next time the FPGA is reconfigured.

Debugging
=========

``west debug`` is supported via the openocd runner. Here is an example for
the :zephyr:code-sample:`hello_world` application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nexys_ganga
   :goals: debug

References
**********

.. _SHAKTI Processor Project:
   https://shakti.org.in/

.. _SHAKTI gc2025 sources:
   https://gitlab.com/shaktiproject/gc2025

.. _Nexys Video Reference Manual:
   https://digilent.com/reference/programmable-logic/nexys-video/reference-manual
