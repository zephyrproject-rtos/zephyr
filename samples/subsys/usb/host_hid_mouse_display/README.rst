..
   Copyright (c) 2026 Antmicro <www.antmicro.com>

.. zephyr:code-sample:: usb-host-hid-mouse-display
   :name: USB Host HID Mouse Display
   :relevant-api: input_interface display_interface

   Draw a mouse cursor marker on a display using a USB HID boot mouse.

Overview
********

This sample demonstrates a USB host application that receives HID boot mouse
events and visualizes them on a display.

The sample:

* Initializes the USB host stack and HID boot class support.
* Receives relative mouse movement and left-button events through Zephyr input.
* Draws a small cross marker at the current pointer position.
* Erases the previously drawn marker before drawing the new one.
* Logs left-button press/release events with current coordinates.

Only relative X/Y movement and the left mouse button are handled by this
application.

Requirements
************

This sample requires:

* A board with USB host support.
* A configured display device (``zephyr,display`` in devicetree ``/chosen``).
* A USB HID boot mouse.

The provided board support is for ``stm32h747i_disco/stm32h747xx/m7``,
using the sparkfun_max3421e shield.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/host_hid_mouse_display
   :board: stm32h747i_disco/stm32h747xx/m7
   :shield: sparkfun_max3421e
   :goals: flash
   :compact:

After the sample starts, connect a USB mouse to the host port.
Moving the mouse updates the marker position on the display.

Sample Output
*************

Example log output:

.. code-block:: console

   [00:00:00.454,000] <inf> nt35510: Init complete(0)
   [00:00:00.455,000] <inf> usbh_hid_boot: HID Boot Device Class initialized
   *** Booting Zephyr OS build v4.4.0-8379-g59f4dda0edd8 ***
   [00:00:00.458,000] <inf> max3421e: REV 0x13, MODE 0xe1, HIEN 0xe3
   [00:00:00.458,000] <inf> main: host: USB host initialized
   [00:00:00.488,000] <inf> max3421e: LS Device connected
   [00:00:00.508,000] <inf> usbh_dev: New device with address 1 state 2
   [00:00:00.521,000] <inf> usbh_dev: Configuration 1 bNumInterfaces 1
   [00:00:00.521,000] <inf> usbh_hid_boot: HID Boot Mouse detected
   [00:00:00.523,000] <inf> usbh_class: Class 'usbh_hid_boot_0' matches interface 0
   [00:00:19.863,000] <inf> main: Cursor X: 16; Y: 1
   [00:00:20.193,000] <inf> main: Skipped 5 messages
   [00:00:20.193,000] <inf> main: Cursor X: 24; Y: 37
   [00:00:20.294,000] <inf> main: Skipped 9 messages
   [00:00:20.294,000] <inf> main: Cursor X: 40; Y: 70
   Left button pressed at x:84 y:128
   Left button let go at x:84 y:128
   [00:00:23.473,000] <inf> main: Cursor X: 49; Y: 118
   [00:00:23.573,000] <inf> main: Skipped 1 messages
   [00:00:23.573,000] <inf> main: Cursor X: 45; Y: 118
   [00:00:23.674,000] <inf> main: Skipped 7 messages
   [00:00:23.674,000] <inf> main: Cursor X: 37; Y: 113
   Left button pressed at x:17 y:103
   Left button let go at x:17 y:103
   [00:00:23.783,000] <inf> main: Skipped 8 messages
   [00:00:23.783,000] <inf> main: Cursor X: 23; Y: 107
   [00:00:23.933,000] <inf> main: Skipped 5 messages

Configuration Options
*********************

This Sample
===========

* :kconfig:option:`CONFIG_SAMPLE_COLORED_OUTPUT` - Highlight LMB event values in green

USB Host and HID Boot
=====================

* :kconfig:option:`CONFIG_USB_HOST_STACK` - Enable USB host stack.
* :kconfig:option:`CONFIG_USBH_HID_BOOT_CLASS` - Enable HID boot class driver.
* :kconfig:option:`CONFIG_USBH_HID_BOOT_INSTANCES_COUNT` - Maximum number of HID boot devices.
* :kconfig:option:`CONFIG_USBH_HID_BOOT_LOG_LEVEL_*` - HID boot driver log level

Display and Input
=================

* :kconfig:option:`CONFIG_DISPLAY` - Enable display subsystem.
* :kconfig:option:`CONFIG_INPUT` - Enable input subsystem.
