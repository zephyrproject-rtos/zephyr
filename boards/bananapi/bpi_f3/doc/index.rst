.. zephyr:board:: bpi_f3

Overview
********

The `Banana Pi`_ BPI-F3 is an industrial-grade RISC-V development board. It is
designed with the SpacemiT K1 SoC, featuring eight X60 cores and a single N308
core. Currently, Zephyr OS supports the N308 core.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Check the `schematic`_ for more details.

+--------+-------------+---------------------+
| Name   | Function    | Usage               |
+========+=============+=====================+
| GPIO47 | R_UART0_TXD | UART Console        |
+--------+-------------+---------------------+
| GPIO48 | R_UART0_RXD | UART Console        |
+--------+-------------+---------------------+
| GPIO70 | PRI_TDI     | JTAG Interface      |
+--------+-------------+---------------------+
| GPIO71 | PRI_TMS     | JTAG Interface      |
+--------+-------------+---------------------+
| GPIO72 | PRI_TCK     | JTAG Interface      |
+--------+-------------+---------------------+
| GPIO73 | PRI_TDO     | JTAG Interface      |
+--------+-------------+---------------------+

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Here is an example for building and flashing the :zephyr:code-sample:`hello_world`
application for the board:

Currently, you need to boot the `Bianbu`_ image provided by the vendor. This
will enable the N308 core and configure uart0 for the N308. Afterward, you can
load the Zephyr application using JTAG.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: bpi_f3/k1/n308
   :goals: flash

After flashing, you should see message similar to the following in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-2578-g6e6daeb40b24 ***
   Hello World! bpi_f3/k1/n308

You can flash the board using an external debug adapter, such as CMSIS-DAP
by default. Specify the interface using the ``OPENOCD_INTERFACE`` environment
variable before running ``west flash``.

For example:

.. code-block:: console

   $ export OPENOCD_INTERFACE=cmsis-dap
   $ west flash

   $ export OPENOCD_INTERFACE=jlink
   $ west flash

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: bpi_f3/k1/n308
   :goals: debug

Step through the application in your debugger, and you should see a message
similar to the following in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-2578-g6e6daeb40b24 ***
   Hello World! bpi_f3/k1/n308

References
**********

.. target-notes::

.. _Banana Pi:
   https://banana-pi.org/

.. _schematic:
    https://docs.banana-pi.org/en/BPI-F3/BananaPi_BPI-F3/

.. _Bianbu:
   https://bianbu.spacemit.com/
