.. _nxp_m2_wifi_bt:

NXP M.2 Wi-Fi/BT Interface
##########################

Overview
********

This shield supports M.2 modules (Murata/ublox etc) to use Bluetooth HCI UART Interface and enable
support for Bluetooth applications and enables SDIO interface for Wi-Fi applications. It supports
M.2 modules for NXP Wireless-Controller SoC IW416 and IW612 which works for both WIFI and Bluetooth.

More information about IW416 & IW612 Wireless Controller can be found at `NXP IW416`_ and `NXP IW612`_

Requirements
************

To use the shield, below requirements needs to be satisfied.

M.2 module with BT HCI UART and SDIO Interface with NXP IW416 or NW612 SoC support.
For Coex (WIFI+BT), UART driver that supports UART RTS line control to wakeup BT CPU from sleep.
To use default Bluetooth-Shell app it needs ~490KB flash & ~130KB RAM memory.
To use default WiFi-Shell app it needs ~1MB flash & ~1.2MB RAM memory.

Integration Platform
********************

The shield is currently supported on,
- :zephyr:board:`mimxrt1060_evk` Rev-C.

Programming
***********

Set ``--shield nxp_m2_wifi_bt`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/handsfree
   :board: mimxrt1060_evk@C//qspi
   :shield: nxp_m2_wifi_bt
   :goals: build

.. zephyr-app-commands::
   :zephyr-app: samples/net/wifi/shell
   :board: mimxrt1060_evk@C//qspi
   :shield: nxp_m2_wifi_bt
   :goals: build

References
**********

.. target-notes::

.. _NXP IW612:
   https://www.nxp.com/products/IW612
.. _NXP IW416:
   https://www.nxp.com/products/IW416
