.. zephyr:code-sample:: rpi-board-info
   :name: Raspberry Pi Board Info

   Retrieve and display Raspberry Pi board information using VideoCore firmware interface.

Overview
********

This sample demonstrates how to query Raspberry Pi board-specific
information from the Raspberry Pi VideoCore firmware interface.

The application retrieves the board revision via the VideoCore firmware mailbox
interface and decodes it into human-readable fields such as:

- Board revision number
- Board type (e.g., Pi 4 Model B rev 1.4)
- Processor type (e.g., BCM2711)
- Installed memory size (e.g., 2GB)
- Manufacturer (e.g., Sony UK)

This showcases how low-level firmware communication can be used to
identify hardware details at runtime.

Requirements
************

This sample requires a Raspberry Pi board, such as:

- Raspberry Pi 4 Model B (``rpi_4b``)

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/raspberrypi/rpi_board_info
   :board: rpi_4b
   :goals: build flash

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0-rc1-337-g31a485318520 ***
   --- Raspberry Pi Board Info ---
   Board revision   : 0x00b03114
   Board type       : Pi 4 Model B rev 1.4
   Processor        : BCM2711
   Memory           : 2GB
   Manufacturer     : Sony UK
   -------------------------------
