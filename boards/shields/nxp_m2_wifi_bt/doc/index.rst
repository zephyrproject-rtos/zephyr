.. _nxp_m2_wifi_bt:

NXP M.2 Wi-Fi/BT Interface
##########################

Overview
********

This shield supports M.2 modules (Murata/ublox etc) to use Bluetooth HCI UART Interface and enable
support for Bluetooth applications and enables SDIO interface for Wi-Fi applications. It supports
M.2 modules for NXP Wireless-Controller SoC IW416 and IW612 which works for both WIFI and Bluetooth.

More information about IW416 & IW612 Wireless Controller can be found at `NXP IW416`_ and `NXP IW612`_

More information about pins assignment of the BT HCI UART can be found at :zephyr:board:`mimxrt1060_evk`

Requirements
************
This shield is currently supported with the NXP :zephyr:board:`mimxrt1060_evk` EVKC revision with M.2 module,
which supports the IW612 or IW416 Wireless Controller for Bluetooth HCI UART and Wi-Fi SDIO enablement.

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
