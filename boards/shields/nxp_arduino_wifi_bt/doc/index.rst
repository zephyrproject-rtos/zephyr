.. _nxp_arduino_wifi_bt:

NXP Arduino WiFi and BT Shield
##############################

Overview
********

The NXP Arduino WiFi/BT shield provides WiFi and Bluetooth connectivity
using NXP wireless SOC modules in Arduino form factor for MCXN based platforms.

Supported Shields
*****************

This shield supports the following configurations:

- ``nxp_arduino_aw510_wifi_bt``: Arduino WiFi/BT Shield with AW510 module (IW416)

Requirements
************

- A board with Arduino connector interface
- UART interface for Bluetooth HCI
- SDIO interface for WiFi

Connections and IOs
===================

The shield uses the following interfaces:

- UART for Bluetooth HCI communication
- SDIO for WiFi communication
- GPIO for control signals (reset, power, wakeup)

Programming
***********

Set ``--shield <option>`` to one of the shield designators shown below when building your application:

- ``nxp_arduino_aw510_wifi_bt``: For Wi-Fi/Bluetooth samples to work with NXP IW416 SoC

For example:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_hr
   :board: frdm_mcxn947/mcxn947/cpu0
   :shield: nxp_arduino_aw510_wifi_bt
   :goals: build

References
**********

.. target-notes::

.. _NXP Wireless Connectivity:
   https://www.nxp.com/products/wireless:WIRELESS-CONNECTIVITY
