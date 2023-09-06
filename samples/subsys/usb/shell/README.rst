.. zephyr:code-sample:: usb-shell
   :name: USB shell
   :relevant-api: usbd_api

   Use shell commands to interact with USB device stack.

Overview
********

The sample enables new experimental USB device support and the shell function.
It is primarily intended to aid in the development and testing of USB controller
drivers and new USB support.

Building and flashing
*********************

Assuming the board has a supported USB device controller, the example can be
built like:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/shell
   :board: reel_board
   :goals: flash
   :compact:

For the USB host functionality a supported host controller is required,
currently it is only MAX3421E. The example can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/shell
   :board: nrf52840dk_nrf52840
   :shield: sparkfun_max3421e
   :gen-args: -DCONFIG_UHC_DRIVER=y -DCONFIG_USB_HOST_STACK=y
   :goals: flash
   :compact:

It is theoretically possible to build USB support using virtual USB controllers
for all platforms, eventually the devicetree overlay has to be adjusted slightly if
the platform has already defined or not `zephyr_uhc0` or `zephyr_udc0` nodelabels.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/shell
   :board: nrf52840dk_nrf52840
   :gen-args: -DEXTRA_CONF_FILE=virtual.conf -DDTC_OVERLAY_FILE=virtual.overlay
   :goals: flash
   :compact:

Sample shell interaction
========================

.. code-block:: console

   uart:~$ usbd defaults
   dev: USB descriptors initialized
   uart:~$ usbd config add 1
   uart:~$ usbd class add foobaz 1
   dev: added USB class foobaz to configuration 1
   uart:~$ usbd init
   dev: USB initialized
   uart:~$ usbh init
   host: USB host initialized
   uart:~$ usbh enable
   host: USB host enabled
   [611:00:28.620,000] <wrn> usbd_core: VBUS detected event
   uart:~$ usbh bus resume
   host: USB bus resumed
   uart:~$ usbd enable
   host: USB device connected
   dev: USB enabled
   uart:~$ usbh device descriptor device 0
   host: transfer finished 0x20006250, err 0
   00000000: 80 06 00 01 00 00 12 00                          |........         |
   bLength			18
   bDescriptorType		1
   bcdUSB			200
   bDeviceClass	        	239
   bDeviceSubClass		2
   bDeviceProtocol		1
   bMaxPacketSize0		64
   idVendor		        2fe3
   idProduct		        ffff
   bcdDevice		        301
   iManufacturer		1
   iProduct		        2
   iSerial			3
   bNumConfigurations           1
