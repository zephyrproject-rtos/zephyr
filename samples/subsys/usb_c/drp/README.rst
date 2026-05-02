.. zephyr:code-sample:: usb-c-drp
   :name: Basic USB-C DRP
   :relevant-api: _usbc_device_api

   Implement a USB-C Power Delivery application in the form of a USB-C DRP (Dual Role Power).

Overview
********

This example demonstrates how to create a USB-C Power Delivery application
using the USB-C subsystem. The application implements a USB-C Dual Role Power
(DRP) device that can operate as either a Source or Sink.

When no device is attached, the DRP device toggles between Source and Sink
roles according to the configured DRP period and duty cycle. Once a device
is attached, the DRP device detects the power role by reading the CC line
resistor advertisement (Rd/Rp) from the port partner and starts the
corresponding Policy Engine state machine (usbc_pe_snk_states for Sink role
or usbc_pe_src_states for Source role). The device can provide power as a
Source (5V at 100mA, 9V at 100mA, or 15V at 100mA) or consume power as a
Sink (5V at 100mA).

.. _usb-c-drp-sample-requirements:

Requirements
************
The TCPC device used by the sample is specified in the devicetree
node that's compatible with ``usb-c-connector``.
The sample has been tested on ``stm32g081b_eval``.
An overlay file is provided.

Hardware Configuration
**********************

This sample uses hardwired connections for ``stm32g081b_eval``
to the UCPD daughterboard for power control (VBUS source enable, DC-DC enable,
VCONN enables, and VBUS discharge control). The specific pin assignments and
their functions are defined in the board overlay file. For detailed pin
mapping information, refer to the STM32G081B-EVAL board datasheet and UCPD
daughterboard documentation.

Building and Running
********************

Build and flash as follows, changing ``stm32g081b_eval`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb_c/drp
   :board: stm32g081b_eval
   :goals: build flash
   :compact:

Connect a Source or Sink device and see console output:

Sample Sink Output
==================

.. code-block:: console

 # DRP Toggling ...
 Unattached.SNK
 Unattached.SRC
 Unattached.SNK
 Unattached.SRC
 # Attach Source Device
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
 RECV 61a1/6
    [0]0a01912c
    [1]0002d12c
    [2]0003c12c
    [3]0004b12c
    [4]000641f4
    [5]c1a42164
 PE_SNK_Evaluate_Capability
 PE_SNK_Select_Capability
 PRL_Tx_Wait_for_PHY_response
 RECV 0161/0
 PRL_Tx_Wait_for_Message_Request
 RECV 03a3/0
 PE_SNK_Transition_Sink
 RECV 05a6/0
 PE_SNK_Ready
 Source Caps:
 PDO 0:0x0a01912c
  Type:                FIXED
  Current:             3000
  Voltage:             5000
  Peak Current:        0
  Uchunked Support:    0
  Dual Role Data:      1
  USB Comms:           0
  Unconstrained Pwr:   1
  USB Suspend:         0
  Dual Role Power:     0
 PDO 1:0x0002d12c
  Type:                FIXED
  Current:             3000
  Voltage:             9000
  Peak Current:        0
  Uchunked Support:    0
  Dual Role Data:      0
  USB Comms:           0
  Unconstrained Pwr:   0
  USB Suspend:         0
  Dual Role Power:     0
 PDO 2:0x0003c12c
  Type:                FIXED
  Current:             3000
  Voltage:             12000
  Peak Current:        0
  Uchunked Support:    0
  Dual Role Data:      0
  USB Comms:           0
  Unconstrained Pwr:   0
  USB Suspend:         0
  Dual Role Power:     0
 PDO 3:0x0004b12c
  Type:                FIXED
  Current:             3000
  Voltage:             15000
  Peak Current:        0
  Uchunked Support:    0
  Dual Role Data:      0
  USB Comms:           0
  Unconstrained Pwr:   0
  USB Suspend:         0
  Dual Role Power:     0
 PDO 4:0x000641f4
  Type:                FIXED
  Current:             5000
  Voltage:             20000
  Peak Current:        0
  Uchunked Support:    0
  Dual Role Data:      0
  USB Comms:           0
  Unconstrained Pwr:   0
  USB Suspend:         0
  Dual Role Power:     0
 PDO 5:0xc1a42164
  Type:                AUGMENTED
  Min Voltage:         3300
  Max Voltage:         21000
  Max Current:         5000
  PPS Power Limited:   0

Sample Source Output
====================

.. code-block:: console

 # DRP Toggling ...
 Unattached.SRC
 Unattached.SNK
 Unattached.SRC
 Unattached.SNK
 # Attach Sink Device
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
 PE_SRC_Discovery
 PE_SRC_Send_Capabilities
 PRL_Tx_Wait_for_PHY_response
 PRL_Tx_Wait_for_Message_Request
 PE_SRC_Discovery
 PE_SRC_Send_Capabilities
 PRL_Tx_Wait_for_PHY_response
 RECV 0a41/0
 PRL_Tx_Wait_for_Message_Request
 RECV 1082/1
    [0]1680292c
 PE_SRC_Negotiate_Capability
 PE_SRC_Transition_Supply
 PRL_Tx_Wait_for_PHY_response
 RECV 0c41/0
 PRL_Tx_Wait_for_Message_Request
 RDO :0x1680292c
  Type:                FIXED
  Object Position:     1
  Giveback:            0
  Capability Mismatch: 1
  USB Comm Capable:    1
  No USB Suspend:      0
  Unchunk Ext MSG Support: 1
  Operating Current:   100 mA
 PE_SRC_Ready
