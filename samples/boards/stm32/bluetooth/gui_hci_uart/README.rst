.. _samples_boards_stm32_bluetooth_gui-hci-uart:

Bluetooth: ST GUI HCI UART
##########################

Overview
*********

Expose the Bluetooth network coprocessor via UART to a PC to be used
with the ST BlueNRG GUI app.

Requirements
************

* A board based on BlueNRG BLE module such as :ref:`disco_l475_iot1_board`
* `BlueNRG GUI`_ application installed on your PC

Default UART settings
*********************

It depends on the board default settings for ``zephyr,bt-c2h-uart`` DTS property.
The UART default settings are:

* Baudrate: 115200 Kbps
* 8 bits, no parity, 1 stop bit

Building and Running
********************

This sample can be found under :zephyr_file:`samples/boards/stm32/bluetooth/gui_hci_uart` in the
Zephyr tree.

.. _BlueNRG GUI:
   https://www.st.com/en/embedded-software/stsw-bnrgui.html
