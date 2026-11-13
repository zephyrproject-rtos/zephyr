.. _sfp_api:

SFP Pluggable Modules
#####################

Overview
********

Small form-factor pluggable (SFP, SFP+, SFP28) transceivers carry a small
management memory that the host reads over I2C. Its layout is defined by
SFF-8472 and is split across two fixed I2C addresses:

* ``0x50`` (A0h), the serial ID page: transceiver type, connector, vendor
  name, part and serial number, wavelength and nominal bit rate. It does
  not change while the module is plugged in.
* ``0x51`` (A2h), the digital diagnostic monitoring (DDM) page: live
  temperature, supply voltage, laser bias current, transmit and receive
  optical power, and the alarm and warning flags for each of them.

The SFP API reads both pages from the module in a cage and returns them as
integer engineering units. The work is split in two layers:

* The SFF-8472 library (:kconfig:option:`CONFIG_SFF8472`) decodes raw page
  bytes into C structures. It does no I/O, uses no floating point and can be
  used on its own, for example to decode a memory dump received over a
  network.
* The SFP driver class reads the pages over I2C for a cage described in
  devicetree and passes them to the library.

Devicetree
**********

A board describes the *cage*, not the module, because a module can be
missing or exchanged at run time. The ``sff,sfp`` binding follows the
binding of the same name in Linux, so the description can be shared:

.. code-block:: devicetree

   sfp0: sfp-cage {
           compatible = "sff,sfp";
           i2c-bus = <&i2c1>;
           mod-def0-gpios = <&gpio0 4 GPIO_ACTIVE_LOW>;
           los-gpios = <&gpio0 5 GPIO_ACTIVE_HIGH>;
           tx-fault-gpios = <&gpio0 6 GPIO_ACTIVE_HIGH>;
           tx-disable-gpios = <&gpio0 7 GPIO_ACTIVE_HIGH>;
   };

``i2c-bus`` is the controller wired to the cage. When ``mod-def0-gpios`` is
given, the driver reports an empty cage with ``-ENODEV``. The other cage
signals are part of the binding so that boards can describe them; the
driver does not use them yet.

Usage
*****

:c:func:`sfp_read_id` returns the decoded serial ID and
:c:func:`sfp_read_diagnostics` the decoded live values. Both can be called
from user mode.

Diagnostics are only decoded for modules with internally calibrated DDM.
Modules without DDM, with externally calibrated DDM, or that need an
address change sequence before A2h can be read return ``-ENOTSUP``, rather
than values that look valid but are not. :c:struct:`sff8472_id` tells which
case applies.

With :kconfig:option:`CONFIG_SFP_SHELL`, the shell provides ``sfp info
<device>`` and ``sfp ddm <device>``:

.. code-block:: console

   uart:~$ sfp ddm sfp-cage
   Temperature: 45.500 C
   Vcc        : 3.300 V
   TX bias    : 6.500 mA
   TX power   : 0.500 mW
   RX power   : 0.400 mW
   Alarms   : 0x211 temp-high tx-bias-high rx-power-low
   Warnings : 0x008 vcc-low

The :zephyr:code-sample:`sfp` sample runs on ``native_sim`` with an emulated
module.

Testing
*******

The ``zephyr,sfp-module-emul`` emulator serves a module image on an emulated
I2C bus, so applications and the driver can be tested without hardware:

.. code-block:: devicetree

   sfp_module: sfp-module@50 {
           compatible = "zephyr,sfp-module-emul";
           reg = <0x50>;
   };

API Reference
*************

.. doxygengroup:: sfp_interface

.. doxygengroup:: sff8472
