.. zephyr:code-sample:: usb-fastboot
   :name: USB Fastboot
   :relevant-api: usbd_api usbd_fastboot

   Demonstrate USB Fastboot device with CDC ACM and Fastboot classes.
   The USB device descriptors are configured via USBD_FASTBOOT_CONFIG_DEFINE
   macro and automatically discovered by the usbd_fastbootd module.

Overview
********

This sample sets up a composite USB device with:
- CDC ACM class for serial console
- Fastboot class for firmware download

The USB descriptors (VID, PID, strings) are defined using the
:c:macro:`USBD_FASTBOOT_CONFIG_DEFINE` convenience macro in application
code, rather than being hardcoded in the subsystem.

Requirements
************

- A board with USB device support
- USB device stack next (``CONFIG_USB_DEVICE_STACK_NEXT=y``)

Building and Running
********************

.. code-block:: console

   west build -b <board> samples/subsys/usb/fastboot
   west flash

After connecting the USB cable, the device should enumerate as:
- A CDC ACM serial port
- A Fastboot device (VID:PID from configuration)
