.. _samples_boards_stm32_bluetooth_interactive-gui:
.. zephyr:code-sample:: st_bluetooth_interactive_gui
   :name: Bluetooth: ST Interactive GUI
   :relevant-api: bluenrg_hci_driver bluetooth

   Expose ST BlueNRG Bluetooth network coprocessor over UART.

Overview
*********

Expose the Bluetooth network coprocessor via UART to a PC to be used
with the ST BlueNRG GUI app. In this case, the main MCU becomes an intermediate level,
and it passes the data between the host (PC) and controller.

Requirements
************

* A board based on BlueNRG BLE module such as :ref:`disco_l475_iot1_board`
* `BlueNRG GUI`_ application installed on your PC

Default UART settings
*********************

It depends on the board default settings for ``zephyr,bt-c2h-uart`` DTS property.
The UART default settings are:

* Baudrate: 115200 bps
* 8 bits, no parity, 1 stop bit

Building and Running
********************

This sample can be found under :zephyr_file:`samples/boards/st/bluetooth/interactive_gui` in the
Zephyr tree.

.. _BlueNRG GUI:
   https://www.st.com/en/embedded-software/stsw-bnrgui.html
