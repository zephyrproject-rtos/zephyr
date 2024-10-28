.. zephyr:code-sample:: usb-c-sink
   :name: Basic USB-C Sink
   :relevant-api: _usbc_device_api

   Implement a USB-C Power Delivery application in the form of a USB-C Sink.

Overview
********

This example demonstrates how to create a USB-C Power Delivery application and
how to generate USB VIF policies in XML format using the USB-C subsystem. The
application implements a USB-C Sink device.

After the USB-C Sink device is plugged into a Power Delivery charger, it
negotiates with the charger to provide 5V@100mA and displays all
Power Delivery Objects (PDOs) provided by the charger.

.. _usb-c-sink-sample-requirements:

Requirements
************
The TCPC device used by the sample is specified in the devicetree
node that's compatible with ``usb-c-connector``.
The sample has been tested on :zephyr:board:`b_g474e_dpow1` and
:zephyr:board:`stm32g081b_eval`. Overlay files for the two boards
are provided.

Building and Running
********************

Build and flash as follows, changing ``b_g474e_dpow1`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb_c/sink
   :board: b_g474e_dpow1
   :goals: build flash
   :compact:

Connect a charger and see console output:

Sample Output
=============

.. code-block:: console

 Unattached.SNK
 AttachWait.SNK
 Attached.SNK
 PE_SNK_Startup
 PRL_INIT
 PRL_HR_Wait_for_Request
 PRL_Tx_PHY_Layer_Reset
 PRL_Tx_Wait_for_Message_Request
 PE_SNK_Discovery
 PE_SNK_Wait_For_Capabilities
 RECV 4161/4 [0]0a01912c [1]0002d12c [2]0004b12c [3]000640e1
 PE_SNK_Evaluate_Capability
 PE_SNK_Select_Capability
 PRL_Tx_Wait_for_PHY_response
 PWR 3A0

 RECV 0363/0
 PRL_Tx_Wait_for_Message_Request
 PE_SNK_Transition_Sink
 RECV 0566/0
 Source Caps:
 PDO 0:
        Type:              FIXED
        Current:           3000
        Voltage:           5000
        Peak Current:      0
        Uchunked Support:  0
        Dual Role Data:    1
        USB Comms:         0
        Unconstrained Pwr: 1
        USB Susspend:      0
        Dual Role Power:   0
 PDO 1:
        Type:              FIXED
        Current:           3000
        Voltage:           9000
        Peak Current:      0
        Uchunked Support:  0
        Dual Role Data:    0
        USB Comms:         0
        Unconstrained Pwr: 0
        USB Susspend:      0
        Dual Role Power:   0
 PDO 2:
        Type:              FIXED
        Current:           3000
        Voltage:           15000
        Peak Current:      0
        Uchunked Support:  0
        Dual Role Data:    0
        USB Comms:         0
        Unconstrained Pwr: 0
        USB Susspend:      0
        Dual Role Power:   0
 PDO 3:
        Type:              FIXED
        Current:           2250
        Voltage:           20000
        Peak Current:      0
        Uchunked Support:  0
        Dual Role Data:    0
        USB Comms:         0
        Unconstrained Pwr: 0
        USB Susspend:      0
        Dual Role Power:   0
 PE_SNK_Ready
