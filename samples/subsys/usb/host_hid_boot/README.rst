..
   Copyright (c) 2026 Antmicro <www.antmicro.com>

.. zephyr:code-sample:: usb-host-hid-boot
   :name: USB Host HID Boot Interface
   :relevant-api: input_interface

   Receive input from USB HID boot interface devices (keyboard and mouse).

Overview
********

This sample demonstrates how to use the USB Host HID (Human Interface Device)
Boot Protocol driver to receive input from USB keyboards and mice connected to
a Zephyr device acting as a USB host.

Upon connection, USB HID boot devices (keyboards and mice) are detected and
configured automatically.

The sample logs the following input events:

* Keyboard key presses and releases (with key names)
* Mouse button events (left, middle, right)
* Mouse movement (X and Y axis)

Requirements
************

This sample uses the USB host stack and requires a USB host controller driver.
Currently only MAX3421E USB host controller on a sparkfun shield connected to STM32H747I-DISCO is tested.

A USB keyboard or mouse that supports the HID Boot Protocol is required. Most
standard USB keyboards and mice support this protocol.

If you want to check if your device support HID Boot enter this command in Linux, while the
device is connected:
:command:`lsusb -v -dVENDOR_ID:PRODUCT_ID`

Where vendor id and product id can be found by executing:
:command:`lsusb`

Is my USB device compatible?
============================

And in the output look for:

.. code-block::

   bInterfaceClass         3 Human Interface Device
   bInterfaceSubClass      1 Boot Interface Subclass

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/host_hid_boot
   :board: stm32h747i_disco/stm32h747xx/m7
   :shield: sparkfun_max3421e
   :goals: flash
   :compact:

Sample Output
*************

When the sample starts, you should see:

.. code-block:: console

   [00:00:00.000,000] <inf> usbh_hid_boot: HID Boot Device Class initialized
   *** Booting Zephyr OS build v4.4.0-8379-ga9bb3097587c ***
   [00:00:00.003,000] <inf> max3421e: REV 0x13, MODE 0xe1, HIEN 0xe3
   [00:00:00.003,000] <inf> main: host: USB host initialized

Keyboard Input
==============

When a USB keyboard is connected and keys are pressed, you will see:

.. code-block:: console

   [00:00:26.565,000] <inf> max3421e: LS Device connected
   [00:00:26.635,000] <inf> usbh_dev: New device with address 1 state 2
   [00:00:26.651,000] <inf> usbh_dev: Configuration 1 bNumInterfaces 2
   [00:00:26.651,000] <inf> usbh_hid_boot: HID Boot Keyboard detected
   [00:00:26.653,000] <inf> usbh_class: Class 'usbh_hid_boot_0' matches interface 0
   Key B was pressed
   Key Left Shift was pressed
   Key 4 was pressed
   Key Left Shift was released
   Key B was released
   Key 4 was released
   Key Left Ctrl was pressed
   Key UP was pressed
   Key UP was released
   Key Left Ctrl was released
   Key ESC was pressed
   Key ESC was released

The sample recognizes all standard keyboard keys including:

* Letter keys (A-Z)
* Number keys (0-9)
* Function keys (F1-F12)
* Modifier keys (Shift, Ctrl, Alt, Meta)
* Special keys (Enter, Space, Backspace, Tab, ESC)
* Navigation keys (Arrow keys, Home, End, Page Up/Down)
* Numpad keys

Mouse Input
===========

When a USB mouse is connected and moved or clicked, you will see:

.. code-block:: console

   [00:01:56.584,000] <inf> max3421e: LS Device connected
   [00:01:56.654,000] <inf> usbh_dev: New device with address 1 state 2
   [00:01:56.667,000] <inf> usbh_dev: Configuration 1 bNumInterfaces 1
   [00:01:56.667,000] <inf> usbh_hid_boot: HID Boot Mouse detected
   [00:01:56.669,000] <inf> usbh_class: Class 'usbh_hid_boot_0' matches interface 0
   Mouse moved in X axis by 1
   Mouse moved in X axis by 1
   Left button pressed
   Mouse moved in Y axis by 1
   Mouse moved in X axis by -1
   Mouse moved in Y axis by 1
   Mouse moved in X axis by -2
   Mouse moved in Y axis by 1
   Left button let go
   Mouse moved in X axis by 1
   Right button pressed
   Right button let go
   Mouse moved in Y axis by -2
   Mouse moved in X axis by 1
   Middle button pressed
   Mouse moved in X axis by 2
   Mouse moved in Y axis by 1
   Middle button let go

The sample reports:

* Left, middle, and right button press and release events
* Mouse movement in X and Y axes with delta values

Notice that scroll wheel events are not supported as they are not defined in HID Boot

Configuration Options
*********************

This Sample
===========

- :kconfig:option:`CONFIG_SAMPLE_COLORED_OUTPUT` - Highlight input event values in green

HID Boot Driver
========================

- :kconfig:option:`CONFIG_USBH_HID_BOOT_CLASS` - Enable USB HID Boot Protocol class driver
- :kconfig:option:`CONFIG_USBH_HID_BOOT_INSTANCES_COUNT` - Maximum number of HID boot devices
- :kconfig:option:`CONFIG_USBH_HID_BOOT_LOG_LEVEL_*` - HID boot driver log level

Problems
********

More than one device is not being detected
==========================================

Make sure that :kconfig:option:`CONFIG_USBH_HID_BOOT_INSTANCES_COUNT` is higher or equal to the
amount of devices that you want to connect.
