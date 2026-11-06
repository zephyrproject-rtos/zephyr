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
   *** Booting Zephyr OS build v4.4.0-15694-gfb3197891bfa ***
   [00:00:00.458,000] <inf> main: USB host initialized
   [00:00:00.458,000] <inf> main: USB host enabled
   [00:00:00.469,000] <inf> main: Display initialized
   [00:00:00.613,000] <inf> usbh_dev: New device with address 1 state 2
   [00:00:00.631,000] <inf> usbh_dev: Configuration 1 bNumInterfaces 1
   [00:00:00.631,000] <inf> usbh_hid_boot: HID Boot Mouse detected
   [00:00:00.635,000] <inf> usbh_class: Class 'usbh_hid_boot_0' matches interface 0
   [00:00:03.115,000] <inf> main: Cursor X: 16; Y: 1
   [00:00:03.285,000] <inf> main: Skipped 2 messages
   [00:00:03.285,000] <inf> main: Cursor X: 16; Y: 1
   [00:00:03.386,000] <inf> main: Skipped 7 messages
   Left button pressed at x:16 y:1
   Left button let go at x:16 y:1
   [00:00:08.145,000] <inf> main: Skipped 9 messages
   [00:00:08.255,000] <inf> main: Cursor X: 16; Y: 1
   [00:00:10.575,000] <inf> main: Skipped 2 messages
   [00:00:10.575,000] <inf> main: Cursor X: 16; Y: 16
   Left button pressed at x:16 y:16
   [00:00:12.905,000] <inf> main: Skipped 4 messages
   [00:00:12.905,000] <inf> main: Cursor X: 16; Y: 16
   [00:00:13.785,000] <inf> main: Cursor X: 18; Y: 16
   [00:00:13.886,000] <inf> main: Skipped 9 messages
   [00:00:13.886,000] <inf> main: Cursor X: 44; Y: 21
   Left button let go at x:71 y:22
   [00:00:14.005,000] <inf> main: Skipped 7 messages
   [00:00:14.005,000] <inf> main: Cursor X: 63; Y: 21
   [00:00:14.105,000] <inf> main: Skipped 6 messages
   [00:00:14.105,000] <inf> main: Cursor X: 71; Y: 22

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
