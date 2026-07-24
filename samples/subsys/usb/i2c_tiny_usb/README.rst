.. zephyr:code-sample:: i2c-tiny-usb
   :name: I2C-Tiny-USB bridge
   :relevant-api: usbd_api i2c_interface

   Expose a Zephyr I2C controller to a host as a USB to I2C adapter.

Overview
********

This sample implements the vendor request protocol of the `I2C-Tiny-USB project`_ on top of the USB
device support and the I2C driver API. It turns a Zephyr board into a USB to I2C adapter that can be
used with the Linux ``i2c-tiny-usb`` `bus driver`_, making the I2C bus of the board accessible from
the host through the ``i2c-dev`` interface and tools like ``i2cdetect``, ``i2cget``, or ``i2cset``.

All communication is done through vendor requests on the default control pipe. The commands to
perform I2C transfers are handled using vendor request nodes, while the commands to get the adapter
capabilities and transfer status are handled by a minimal USB function with a vendor specific
interface and no endpoints. I2C messages belonging to the same transaction are collected and
executed in a single I2C transfer, which preserves repeated start conditions between the messages.

Any I2C controller driver can be used, including the GPIO bit-banging driver
:dtcompatible:`gpio-i2c` on boards without a free hardware I2C controller. The I2C controller is
selected with the ``zephyr_i2c`` devicetree node label, which many boards already provide. On a
board without it, a devicetree overlay can attach the label to any I2C controller, for example
``zephyr_i2c: &arduino_i2c {};``.

Requirements
************

This project requires a USB device controller driver using the UDC API and an I2C controller with
the ``zephyr_i2c`` devicetree node label.

On the host side you need:

* A Linux kernel with the ``i2c-tiny-usb`` driver (``CONFIG_I2C_TINY_USB``), which is enabled in
  most distribution kernels.
* The ``i2c-tools`` package, which provides the ``i2cdetect``, ``i2cget``, and ``i2cset`` commands
  used below. On Debian and Ubuntu install it with ``sudo apt install i2c-tools``, on Fedora with
  ``sudo dnf install i2c-tools``.

Building and Running
********************

Build and flash the sample with:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/i2c_tiny_usb
   :board: adafruit_feather_rp2040
   :goals: flash
   :compact:

On the :zephyr:board:`adafruit_feather_rp2040`, ``zephyr_i2c`` is the controller routed to the
STEMMA QT connector, so any STEMMA QT or Qwiic breakout can be plugged in without additional wiring.

The sample uses the vendor and product ID of the original I2C-Tiny-USB project, so the
``i2c-tiny-usb`` driver binds to the device automatically when it is connected and registers a new
I2C adapter:

.. code-block:: console

   $ sudo dmesg | grep i2c-tiny-usb
   i2c-tiny-usb 1-2.1:1.0: version 1.05 found at bus 001 address 016
   i2c i2c-20: connected i2c-tiny-usb device

   $ i2cdetect -l | grep tiny
   i2c-20  i2c   i2c-tiny-usb at bus 001 device 016    I2C adapter

The adapter shows up as ``i2c-N``, where ``N`` is a bus number the kernel assigns dynamically
(``i2c-20`` in the output above, but it may differ on your system and can change when the device is
re-plugged). That number ``N`` is what you pass to the ``i2c-tools`` commands as the bus argument.
Substitute your own number for ``20`` in the examples below.

Now the I2C bus of the board can be used from the host, for example to scan for connected devices or
to read the manufacturer ID register of a sensor found at address 0x1c:

.. code-block:: console

   $ sudo i2cdetect -y 20
        0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
   00:                         -- -- -- -- -- -- -- --
   10: -- -- -- -- -- -- -- -- -- -- -- -- 1c -- -- --
   ...

   $ sudo i2cget -y 20 0x1c 0x0d
   0xc7

Limitations
***********

The transfers of a transaction are only executed when the host announces the last message or
requests read data. A NAK from a deferred write is therefore reported with the status of the whole
transaction rather than with the message that caused it. Transactions addressing multiple different
targets are not supported, such transactions are rejected and reported as failed.

References
**********

.. target-notes::

.. _I2C-Tiny-USB project: https://github.com/harbaum/I2C-Tiny-USB
.. _bus driver: https://elixir.bootlin.com/linux/latest/source/drivers/i2c/busses/i2c-tiny-usb.c
