.. _testusb-app:

Testusb application sample
##########################

The testusb sample implements a loopback function. This function can be used
to test USB device drivers and the device stack connected to a Linux host
and has a similar interface to "Gadget Zero" of the Linux kernel.
The userspace tool ``testusb`` is needed to start the tests.

Building and flashing
*********************

Follow the general procedure for building and flashing Zephyr device.

Testing
*******

To run USB tests:

#. Load the ``usbtest`` Linux kernel module on the Linux Host.

   .. code-block:: console

      $ sudo modprobe usbtest vendor=0x2fe3 product=0x0009

   The ``usbtest`` module should claim the device:

   .. code-block:: console

      [21746.128743] usb 9-1: new full-speed USB device number 16 using uhci_hcd
      [21746.303051] usb 9-1: New USB device found, idVendor=2fe3, idProduct=0009, bcdDevice= 2.03
      [21746.303055] usb 9-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
      [21746.303058] usb 9-1: Product: Zephyr testusb sample
      [21746.303060] usb 9-1: Manufacturer: ZEPHYR
      [21746.303063] usb 9-1: SerialNumber: 86FE679A598AC47A
      [21746.306149] usbtest 9-1:1.0: matched module params, vend=0x2fe3 prod=0x0009
      [21746.306153] usbtest 9-1:1.0: Generic USB device
      [21746.306156] usbtest 9-1:1.0: full-speed {control} tests

#. Use the ``testusb`` tool in ``linux/tools/usb`` inside Linux kernel source directory
   to start the tests.

   .. code-block:: console

      $ sudo ./testusb -D /dev/bus/usb/009/016
      /dev/bus/usb/009/016 test 0,    0.000007 secs
      /dev/bus/usb/009/016 test 9,    4.994475 secs
      /dev/bus/usb/009/016 test 10,   11.990054 secs

#. To run all the tests the Zephyr's VID / PID should be inserted to USB
   driver id table. The method for loading the ``usbtest`` driver for our
   device is described here: https://lwn.net/Articles/160944/.

   Since we use the "Gadget Zero" interface we specify reference device
   ``0525:a4a0``.

   .. code-block:: console

      $ sudo sh -c "echo 0x2fe3 0x0009 0 0x0525 0xa4a0 > /sys/bus/usb/drivers/usbtest/new_id"

#. Use the ``testusb`` tool in ``linux/tools/usb`` inside Linux kernel source directory
   to start the tests.

   .. code-block:: console

      $ sudo ./testusb -v 512 -D /dev/bus/usb/009/016
      /dev/bus/usb/009/017 test 0,    0.000008 secs
      /dev/bus/usb/009/017 test 1,    2.000001 secs
      /dev/bus/usb/009/017 test 2,    2.003058 secs
      /dev/bus/usb/009/017 test 3,    1.054082 secs
      /dev/bus/usb/009/017 test 4,    1.001010 secs
      /dev/bus/usb/009/017 test 5,   57.962142 secs
      /dev/bus/usb/009/017 test 6,   35.000096 secs
      /dev/bus/usb/009/017 test 7,   30.000063 secs
      /dev/bus/usb/009/017 test 8,   18.000159 secs
      /dev/bus/usb/009/017 test 9,    4.984975 secs
      /dev/bus/usb/009/017 test 10,   11.991022 secs
      /dev/bus/usb/009/017 test 11,   17.030996 secs
      /dev/bus/usb/009/017 test 12,   17.103034 secs
      /dev/bus/usb/009/017 test 13,   18.022084 secs
      /dev/bus/usb/009/017 test 14,    2.458976 secs
      /dev/bus/usb/009/017 test 17,    2.001089 secs
      /dev/bus/usb/009/017 test 18,    1.998975 secs
      /dev/bus/usb/009/017 test 19,    2.010055 secs
      /dev/bus/usb/009/017 test 20,    1.999911 secs
      /dev/bus/usb/009/017 test 21,    2.440972 secs
      /dev/bus/usb/009/017 test 24,   55.112078 secs
      /dev/bus/usb/009/017 test 27,   56.911052 secs
      /dev/bus/usb/009/017 test 28,   34.163089 secs
      /dev/bus/usb/009/017 test 29,    3.983999 secs
