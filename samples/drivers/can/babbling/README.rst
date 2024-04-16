.. zephyr:code-sample:: can-babbling
   :name: Controller Area Network (CAN) babbling node
   :relevant-api: can_interface

   Simulate a babbling CAN node.

Overview
********

In a Controller Area Network a babbling node is a node continuously (and usually erroneously)
transmitting CAN frames with identical - often high - priority. This constant babbling blocks CAN
bus access for any CAN frame with lower priority as these frames will loose the bus arbitration.

This sample application simulates a babbling CAN node. The properties of the CAN frames sent are
configurable via :ref:`Kconfig <kconfig>`. The frames carry no data as only the arbitration part of
the CAN frame is of interest.

Being able to simulate a babbling CAN node is useful when examining the behavior of other nodes on
the same CAN bus when they constantly loose bus arbitration.

The source code for this sample application can be found at:
:zephyr_file:`samples/drivers/can/babbling`.

Requirements
************

This sample requires a board with a CAN controller. The CAN controller must be configured using the
``zephyr,canbus`` :ref:`devicetree <dt-guide>` chosen node property.

The sample supports an optional button for stopping the babbling. If present, the button must be
configured using the ``sw0`` :ref:`devicetree <dt-guide>` alias, usually in the :ref:`BOARD.dts file
<devicetree-in-out-files>`.

Building and Running
********************

Example building for :ref:`twr_ke18f`:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/can/babbling
   :board: twr_ke18f
   :goals: build flash

Sample output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.1.0-4606-g8c1efa8b96bb  ***
   babbling on can@40024000 with standard (11-bit) CAN ID 0x010, RTR 0, CAN-FD 0
   abort by pressing User SW3 button
