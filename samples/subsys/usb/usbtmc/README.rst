.. zephyr:code-sample:: usbtmc
   :name: USBTMC instrument
   :relevant-api: usbd_api usbd_usbtmc

   Implement a tiny laboratory instrument using the USBTMC class.

Overview
********

This sample demonstrates how to implement a test and measurement instrument
using the USBTMC device class. It can run on any board with a USB device
controller. The sample implements a deliberately small SCPI-like command
parser on top of the USBTMC transport and supports the following commands:

* ``*IDN?`` returns the instrument identification
* ``*RST`` resets the instrument state
* ``MEAS:TEMP?`` returns a synthesized temperature measurement
* ``SYST:UPT?`` returns the uptime in seconds
* ``SYST:ERR?`` returns and removes the oldest error from the error queue

The identification response is put together from the strings that are also
used for the USB device descriptors, a serial number obtained using the
:ref:`hwinfo_api` when :kconfig:option:`CONFIG_HWINFO` is enabled, and the
kernel version as the firmware level.

If the board has a user LED, it is turned on for a human recognizable amount
of time when the host sends an INDICATOR_PULSE request.

Building and Running
********************

The code can be found in :zephyr_file:`samples/subsys/usb/usbtmc`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/usbtmc
   :board: frdm_rw612
   :goals: build flash
   :compact:

The sample has been verified on :zephyr:board:`frdm_rw612`, using the USB
device connector of the board rather than the debug connector.

Using the instrument
********************

USBTMC instruments are normally accessed through a VISA implementation. The
examples below use `PyVISA`_ with the pure Python `PyVISA-py`_ backend, which
works the same way on all three host operating systems:

.. code-block:: python

   import pyvisa

   rm = pyvisa.ResourceManager("@py")
   inst = rm.open_resource(rm.list_resources("USB?*::INSTR")[0])
   print(inst.query("*IDN?"))
   print(inst.query("MEAS:TEMP?"))

Host specific notes and alternatives follow.

.. tabs::

   .. group-tab:: Linux

      The Linux kernel USBTMC host driver binds to the device and provides a
      character device such as :file:`/dev/usbtmc0`. Commands can be sent and
      responses read directly from a shell:

      .. code-block:: console

         $ echo "*IDN?" > /dev/usbtmc0
         $ cat /dev/usbtmc0
         Zephyr Project,USBD USBTMC Sample,F8E9A2C3D4B5A697,4.5.99

      Accessing the device as a regular user requires a udev rule granting
      access to :file:`/dev/usbtmc*`, otherwise the commands above have to be
      run as root.

      PyVISA-py can also talk to the device directly over USB, which bypasses
      the kernel driver and requires permission to access the raw USB device.

   .. group-tab:: macOS

      macOS has no USBTMC kernel driver, so there is no device node to write
      to. Use PyVISA-py, which reaches the device through `PyUSB`_ and libusb:

      .. code-block:: console

         $ brew install libusb
         $ pip install pyvisa pyvisa-py pyusb

      No driver installation or code signing is needed, because no kernel
      driver claims the USBTMC interface.

      Note that PyVISA-py reports the vendor and product IDs in decimal in the
      resource string, for example
      ``USB0::12259::20::F8E9A2C3D4B5A697::0::INSTR``, while VISA
      implementations on other platforms conventionally use hexadecimal.

   .. group-tab:: Windows

      USBTMC devices are supported by established VISA implementations, such as
      NI-VISA or Keysight IO Libraries Suite. The device appears as a USB
      instrument, for example in NI MAX or Keysight Connection Expert, and can
      be queried interactively or from any VISA application using a resource
      string like ``USB0::0x2FE3::0x0014::F8E9A2C3D4B5A697::INSTR``.

      To use PyVISA-py instead, the WinUSB driver has to be assigned to the
      device first, for example with `Zadig`_.

.. _PyVISA: https://pyvisa.readthedocs.io/
.. _PyVISA-py: https://pyvisa.readthedocs.io/projects/pyvisa-py/
.. _PyUSB: https://pyusb.github.io/pyusb/
.. _Zadig: https://zadig.akeo.ie/
