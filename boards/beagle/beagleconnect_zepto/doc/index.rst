.. zephyr:board:: beagleconnect_zepto

Overview
********

BeagleConnect Zepto is a low-cost microcontroller development board from
BeagleBoard.org based on the Texas Instruments MSPM0L1117 microcontroller.
The board is designed for embedded development with Zephyr RTOS and provides
a compact platform featuring expansion through mikroBUS-compatible add-on
modules and QWIIC connectors.

The `MSPM0L1117`_ features an Arm Cortex-M0+ CPU running at up to 32 MHz,
128 KB of on-chip flash memory, and 16 KB of SRAM.

Hardware
********

BeagleConnect Zepto provides the following hardware features:

+ Texas Instruments `MSPM0L1117`_ Arm Cortex-M0+ MCU
+ 128 KB Flash memory
+ 16 KB SRAM
+ mikroBUS-compatible expansion connector
+ 12 non-mikroBUS pins for additional capabilities
+ Two QWIIC expansion connectors
+ RGB LED
+ Blue user LED
+ UART connectivity through an external USB-to-UART adapter
+ Raspberry Pi or `BeagleY-AI`_ HAT compatible connector.

Supported Features
==================

.. zephyr:board-supported-hw::

Flashing
********

Build the :zephyr:code-sample:`hello_world` sample:

.. zephyr-app-commands::
   :board: beagleconnect_zepto/mspm0l1117
   :zephyr-app: samples/hello_world
   :goals: build

This builds the program and the binary is present in the :file:`build/zephyr` directory as
:file:`zephyr.bin`.

If BeagleConnect Zepto is connected over I2C or UART, we flash this binary using
`BeagleBoard Imaging Utility`_.
The Raspberry Pi and `BeagleY-AI`_ HAT compatible connector can be used to flash from those boards
without needing extra wires.

Alternatively, when an XDS110 debug probe is connected to the SWD interface,
the application can be flashed directly using:

.. zephyr-app-commands::
   :board: beagleconnect_zepto/mspm0l1117
   :goals: flash

Debugging
*********

The board supports XDS110 over SWD for debugging.

.. zephyr-app-commands::
   :board: beagleconnect_zepto/mspm0l1117
   :goals: debug

References
**********

.. _MSPM0L1117:
   https://www.ti.com/product/MSPM0L1117
.. _BeagleY-AI:
   https://www.beagleboard.org/boards/beagley-ai
.. _BeagleBoard Imaging Utility:
   https://beagleboard.github.io/bb-imager-rs/bb-imager/1.0.8/install-gui.html
