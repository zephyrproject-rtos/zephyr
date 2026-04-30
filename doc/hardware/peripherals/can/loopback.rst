.. _can_loopback:

CAN Loopback Driver
###################

Overview
********

The CAN loopback driver (:dtcompatible:`zephyr,can-loopback`) is a virtual CAN
driver that loops back all transmitted frames to registered receivers on the
same device. No physical CAN hardware is required.

It is primarily used for:

* Testing CAN application logic without hardware
* Unit testing CAN drivers and protocol stacks
* Development on platforms without CAN peripherals

Configuration
*************

To enable the CAN loopback driver, add the following to your project
configuration:

.. code-block:: kconfig

   CONFIG_CAN_LOOPBACK=y

Device Tree
***********

The loopback driver must be enabled in the device tree:

.. code-block:: devicetree

   / {
       can_loopback0: can_loopback {
           compatible = "zephyr,can-loopback";
           status = "okay";
       };
   };

The chosen node must also reference it:

.. code-block:: devicetree

   / {
       chosen {
           zephyr,canbus = &can_loopback0;
       };
   };

API Reference
*************

The loopback driver implements the standard :ref:`CAN controller API <can_api>`.