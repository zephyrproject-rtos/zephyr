.. _usb_cdc-acm:

USB CDC ACM Peformance Sample Application
#########################################

Overview
********

This sample tests the performance of the USB Communication Device Class (CDC)
Abstract Control Model (ACM) driver provided by the Zephyr project.
This sample can be found under :zephyr_file:`samples/subsys/usb/cdc_acm_perf` in the
Zephyr project tree.

This sample firmware streams incrementing uint32 data out the serial port as fast as possible.
An included host application can be run on a PC to read this data, check for any errors,
and measure the data rate.

Requirements
************

This project requires an USB device driver, which is available for multiple
boards supported in Zephyr.

Running
*******

Plug the board into a host device, for example, a PC running Linux.
The board will be detected as shown by the Linux dmesg command:

.. code-block:: console

   usb 9-1: new full-speed USB device number 112 using uhci_hcd
   usb 9-1: New USB device found, idVendor=8086, idProduct=f8a1
   usb 9-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
   usb 9-1: Product: CDC-ACM
   usb 9-1: Manufacturer: Intel
   usb 9-1: SerialNumber: 00.01
   cdc_acm 9-1:1.0: ttyACM1: USB ACM device

The included host application can be used to read the data. To build and start
the host application, run the following:

.. code-block:: console

   cd samples/subsys/usb/cdc_acm_perf/host/c
   cmake .
   ./usb-cdc-perf <zephyr serial port>

Once the device connects to the host, it will start streaming data and the host
app will display stats on data received.

A Go version of the app is also included.

Troubleshooting
===============

If the ModemManager runs on your operating system, it will try
to access the CDC ACM device and maybe you can see several characters
including "AT" on the terminal attached to the CDC ACM device.
You can add or extend the udev rule for your board to inform
ModemManager to skip the CDC ACM device.
For this example, it would look like this:

.. code-block:: none

   ATTRS{idVendor}=="8086" ATTRS{idProduct}=="f8a1", ENV{ID_MM_DEVICE_IGNORE}="1"

You can use
``/lib/udev/rules.d/77-mm-usb-device-blacklist.rules`` as reference.
