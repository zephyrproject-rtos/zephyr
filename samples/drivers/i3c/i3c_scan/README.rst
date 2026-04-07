.. zephyr:code-sample:: i3c_scan
   :name: I3C Bus Scan
   :relevant-api: i3c_interface

   Discover and display all I3C and I2C devices on an I3C bus.

Overview
********

This sample discovers all I3C and legacy I2C devices attached to an I3C bus
and displays their properties per the MIPI I3C Basic v1.1.1 specification.

Device discovery happens automatically during I3C controller initialization
via the standard ``i3c_bus_init()`` sequence:

#. OD slow-speed first broadcast (disables I2C spike filters per Section 4.3.2.2.2)
#. RSTACT broadcast (optional, ``CONFIG_I3C_INIT_RSTACT``)
#. RSTDAA broadcast (reset all dynamic addresses)
#. DISEC broadcast (disable events during DAA)
#. SETDASA (for devicetree-known devices with static addresses)
#. SETAASA (for devicetree-known devices that support it)
#. ENTDAA (Dynamic Address Assignment for all remaining targets)
#. ENEC broadcast (re-enable Hot-Join events)

Unknown I3C targets discovered via ENTDAA are allocated from the descriptor
memory pool (``CONFIG_I3C_NUM_OF_DESC_MEM_SLABS``).

For each discovered device the sample prints:

- I3C targets: PID (Vendor/Part/Instance IDs), BCR, DCR, dynamic and static
  addresses, MRL, MWL, and max IBI payload size.
- Legacy I2C devices: 7-bit address and LVR (Legacy Virtual Register).

Requirements
************

- A board with an I3C controller supported by Zephyr.
- Optionally one or more I3C or I2C target devices connected to the bus.

Building and Running
********************

.. code-block:: console

   west build -b <board> samples/drivers/i3c/scan
   west flash

If your board does not enable the I3C controller by default, create a
board overlay to enable it:

.. code-block:: devicetree

   / {
       aliases {
           i3c0 = &i3c0;
       };
   };

   &i3c0 {
       status = "okay";
   };

For boards with many I3C devices, increase the descriptor pool via
board-specific configuration:

.. code-block:: cfg

   CONFIG_I3C_NUM_OF_DESC_MEM_SLABS=8

Sample Output
*************

.. code-block:: console

   *** I3C Bus Discovery Sample ***
   Controller: i3c0@49034000
   MIPI I3C Specification v1.1.1 Compliant
   Max descriptor pool: I3C=3, I2C=3
   Bus init already completed (DAA at driver init)

   *** I3C Bus Scan Starting ***

   --- Scanning I3C Devices ---
   ========================================
   I3C Device #1:
     Device Name: unknown-10
     Transport Mode: I3C
   ----------------------------------------
     PID: 0x046a00000000
       - Vendor ID: 0x6a (106)
       - Part ID: 0x0000 (0)
       - Instance ID: 0x0000 (0)
     Device Type: Vendor Defined (DCR=0x44)
     BCR: 0x27
       - Role: Target
   ========================================

   --- Scanning I2C/Legacy Devices ---
   No I2C devices found on the bus.

   *** Scan Complete ***
   Total I3C devices found: 1
   Total I2C devices found: 0
   Total devices: 1
