.. _rnbd451_add_on_shield:

RNBD451 Add-on Board
###############################

Overview
********

The RNBD451 Add-on Board is an efficient low-cost development platform to evaluate and demonstrate
the features, capabilities and interfaces of Microchip's Bluetooth® Low Energy module, RNBD451PE.
The add-on board is compliant to the mikroBUS™ standard and includes an on-board MCP2200 USB-to-UART
converter enabling out-of-box evaluation with no other hardware requirements.

.. figure:: img/rnbd451-add-on-board.webp
   :height: 350px
   :align: center
   :alt: RNBD451 Add-on Board

   **Figure 1: RNBD451 Add-on Board**

Requirements
************

This shield is compatible with a development board that includes a mikroBUS™ socket.
If a mikroBUS socket is not available, a mikroBUS adapter can be used to provide both an
extension header and a mikroBUS interface. The mikroBUS™ Xplained Pro is an extension Board
for the Xplained Pro evaluation platform,  designed to support mikroBUS™ Click boards™ with
Xplained Pro MCU boards.


.. figure:: img/2080-atmbusadapter-xpro.webp
   :height: 300px
   :align: center
   :alt: ATMBUSADAPTER-XPRO - mikroBUS Adapter

   **Figure 2: ATMBUSADAPTER-XPRO - mikroBUS Adapter**

Configurations
**************

  When bringing up the RNBD451 Bluetooth Low Energy (BLE) controller after a Power-On Reset (POR),
  the Zephyr Bluetooth subsystem sends the **HCI reset command** (the first HCI command) to the BLE
  controller. However, in the original design, this command is sent too early for the RNBD451, as
  it takes more than 400 ms from POR to be ready to accept the first HCI command.

  To address this timing issue, introduce a delay in the Zephyr application to enable Bluetooth.
  This delay ensures that the **HCI reset command** is sent at the appropriate time. Add the following
  code snippet to the application to introduce a 500 ms delay before calling ``bt_enable()``:

  ``k_sleep(K_MSEC(500));``

Programming
***********

Activate the presence of the shield for the project build by adding the
``--shield mchp_rnbd451_bt_mikrobus`` or ``--shield mchp_rnbd451_bt_xplained_pro`` when you invoke
``west build`` based on Mikrobus or Xplained Pro interface:

.. zephyr-app-commands::
   :app: samples/bluetooth/peripheral
   :board: your_board_name
   :shield: shield mchp_rnbd451_bt_mikrobus
   :goals: build

Or

.. zephyr-app-commands::
   :app: samples/bluetooth/peripheral
   :board: your_board_name
   :shield: mchp_rnbd451_bt_xplained_pro
   :goals: build

References
**********

.. target-notes::

.. _RNBD451 Add-on Board Primary User Guide:
   https://ww1.microchip.com/downloads/aemDocuments/documents/WSG/ProductDocuments/UserGuides/RNBD451-Add-On-Board-User-Guide-DS50003476.pdf
