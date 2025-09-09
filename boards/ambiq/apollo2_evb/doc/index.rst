.. zephyr:board:: apollo2_evb

The Apollo2 EVB is a board by Ambiq featuring their ultra-low power Apollo2 SoC.

Hardware
********

The Ambiq Apollo2 SoC provides the following hardware features:

- ARM® Cortex® M4F core with a Floating Point Unit
- Up to 48 MHz operating frequency
- 16 kB 2-way Associative Cache
- Up to 1 MB of flash memory for code/data
- Up to 256 KB of low leakage RAM for code/data

For more information about the Apollo2 SoC and a potential Apollo2 EVB board:

* `Apollo2 SoC Website <https://ambiq.com/apollo2/>`_
* `Apollo2 SoC Datasheet <https://www.fujitsu.com/uk/imagesgig5/Apollo2_Blue_MCU_Data_Sheet_rev0p8.pdf>`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
=========================

Flashing an application
-----------------------

Connect your device to your host computer using the JLINK USB port.
The sample application :zephyr:code-sample:`hello_world` is used for this example.
Build the Zephyr kernel and application, then flash it to the device:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: apollo2_evb
   :goals: flash

.. note::
   ``west flash`` requires `SEGGER J-Link software`_ and `pylink`_ Python module
   to be installed on you host computer.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should be able to see on the corresponding Serial Port
the following message:

.. code-block:: console

   Hello World! apollo2_evb

.. _SEGGER J-Link software:
   https://www.segger.com/downloads/jlink

.. _pylink:
   https://github.com/Square/pylink
