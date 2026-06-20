.. _can_loopback:

CAN Loopback Driver
###################

.. contents::
    :local:
    :depth: 2

Overview
********

The CAN loopback driver (``zephyr,can-loopback``) is a virtual CAN controller
that loops back all transmitted frames to any registered receivers without
requiring real CAN hardware. It is used for testing CAN-based applications and
protocol stacks entirely in software, including on :zephyr:board:`native_sim`.

The loopback driver implements the full :ref:`CAN controller API <can_api>` and
supports both Classical CAN and CAN-FD frame formats.

Use cases include:

* Unit testing CAN drivers and protocol stacks without physical hardware
* Testing ISO-TP, CANopen, and other higher-level CAN protocols in CI pipelines
* Developing and debugging CAN applications on the host machine

Configuration
*************

To enable the CAN loopback driver, add the following to your ``prj.conf``:

.. code-block:: kconfig

   CONFIG_CAN=y
   CONFIG_CAN_LOOPBACK=y

The loopback device is defined in devicetree using the
:dtcompatible:`zephyr,can-loopback` compatible string:

.. code-block:: devicetree

   / {
       can_loopback0: can-loopback0 {
           compatible = "zephyr,can-loopback";
           status = "okay";
       };
   };

For CAN-FD support, add the ``can-fd`` property:

.. code-block:: devicetree

   / {
       can_loopback0: can-loopback0 {
           compatible = "zephyr,can-loopback";
           status = "okay";
           can-fd;
       };
   };

Usage
*****

The CAN loopback driver is accessed using the standard CAN controller API.
Obtain the device reference using the devicetree label:

.. code-block:: c

   #include <zephyr/kernel.h>
   #include <zephyr/drivers/can.h>

   const struct device *can_dev = DEVICE_DT_GET(DT_NODELABEL(can_loopback0));

   if (!device_is_ready(can_dev)) {
       printk("CAN loopback device not ready\n");
       return -ENODEV;
   }

Start the CAN controller before sending or receiving frames:

.. code-block:: c

   int ret = can_start(can_dev);
   if (ret != 0) {
       printk("Failed to start CAN controller: %d\n", ret);
       return ret;
   }

Sending a Frame
===============

.. code-block:: c

   struct can_frame frame = {
       .flags = 0,
       .id = 0x123,
       .dlc = 2,
       .data = {0xDE, 0xAD},
   };

   ret = can_send(can_dev, &frame, K_MSEC(100), NULL, NULL);
   if (ret != 0) {
       printk("Failed to send CAN frame: %d\n", ret);
   }

Receiving a Frame
=================

Add a filter to receive frames matching a specific CAN ID:

.. code-block:: c

   struct can_filter filter = {
       .flags = 0,
       .id = 0x123,
       .mask = CAN_STD_ID_MASK,
   };

   void rx_callback(const struct device *dev, struct can_frame *frame,
                    void *user_data)
   {
       printk("Received CAN frame: id=0x%x dlc=%d\n",
              frame->id, frame->dlc);
   }

   int filter_id = can_add_rx_filter(can_dev, rx_callback, NULL, &filter);
   if (filter_id < 0) {
       printk("Failed to add CAN RX filter: %d\n", filter_id);
   }

Building and Running
********************

Build for ``native_sim`` to run entirely on the host machine:

.. code-block:: console

   west build -p auto -b native_sim samples/drivers/can/counter

The loopback driver is also used in Zephyr's CAN API test suite:

.. zephyr-app-commands::
   :zephyr-app: tests/drivers/can/api
   :board: native_sim
   :goals: build run

API Reference
*************

See :ref:`can_api` for the full CAN controller API used with this driver.
