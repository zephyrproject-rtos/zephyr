.. _usb-c-source-sample:

Basic USB-C SOURCE
##################

Overview
********

This example demonstrates how to create a USB-C Power Delivery application
using the USB-C subsystem. The application implements a USB-C Source device.

After a USB-C Sink device is plugged into the USB-C Source device, it
negotiates with the Source device to provide 5V at 100mA, 9V at 100mA,
or 15V at 100mA and the Requested Data Object (RDO) provided by the Source
is displayed.

.. _usb-c-source-sample-requirements:

Requirements
************
The TCPC device used by the sample is specified in the devicetree
node that's compatible with ``usb-c-connector``.
The sample has been tested on ``stm32g081b_eval``.
An overlay file is provided.

Building and Running
********************

Build and flash as follows, changing ``stm32g081b_eval`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb-c/source
   :board: stm32g081b_eval
   :goals: build flash
   :compact:

Connect a sink device and see console output:

Sample Output
=============

.. code-block:: console

 PRL_TX_SUSPEND
 PRL_HR_SUSPEND
 PE_SUSPEND
 UnattachedWait.SRC
 Unattached.SRC
 AttachWait.SRC
 Attached.SRC
 PE_SRC_Startup
 PRL_INIT
 PRL_HR_Wait_for_Request
 PRL_Tx_PHY_Layer_Reset
 PRL_Tx_Wait_for_Message_Request
 PE_SRC_Send_Capabilities
 PRL_Tx_Wait_for_PHY_response
 PRL_Tx_Wait_for_Message_Request
 PE_SRC_Discover
 PE_SRC_Send_Capabilities
 PRL_Tx_Wait_for_PHY_response
 RECV 1042/1
    [0]1300280a
 PRL_Tx_Wait_for_Message_Request
 PE_SRC_Negotiate_Capability
 PE_SRC_Transition_Supply
 PRL_Tx_Wait_for_PHY_response
 RECV 0441/0
 PRL_Tx_Wait_for_Message_Request
 PE_SRC_Ready
 PRL_Tx_Wait_for_PHY_response
 RECV 0641/0
 PRL_Tx_Wait_for_Message_Request
 REQUEST RDO: 1300280a
  Object Position:         1
  Giveback:                0
  Capability Mismatch:     0
  USB Comm Capable:        1
  No USB Suspend:          1
  Unchunk Ext MSG Support: 0
  Operating Current:       100 mA
  Min Operating Current:   100 mA
