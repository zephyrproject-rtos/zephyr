:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.6:

Migration guide to Zephyr v4.6.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.5.0 to
Zephyr v4.6.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.6>`.

.. contents::
    :local:
    :depth: 2

Common
******

Build System
************

Kernel
******

Boards
******

Device Drivers and Devicetree
*****************************

.. Only place contents common to all device drivers here. Contents specific to one driver subsystem
   goes into its own subsection, below.

.. Group contents in this section by subsystem, e.g.:
..
.. ADC
.. ===
..
.. ...

.. zephyr-keep-sorted-start re(^\w) ignorecase

ADC
===

* :dtcompatible:`nxp,lpc-lpadc` channels that use ``ADC_GAIN_1_2`` for the 30/64 partial scale
  should use ``ADC_GAIN_15_32``: with ``ADC_GAIN_1_2``, which still selects that scale, the
  conversions of :c:func:`adc_raw_to_millivolts_dt` and :c:func:`adc_raw_to_millivolts` are
  6.25% too low.

Ethernet
========

* :kconfig:option:`CONFIG_ETH_PHY_DRIVER` is no longer enabled by default. It is now selected by
  the Ethernet drivers that need a PHY driver. Out-of-tree Ethernet drivers that use a PHY driver
  must select it as well. (:github:`120871`)

GPIO
====

* The standalone Raspberry Pi RP1 GPIO driver (``CONFIG_GPIO_RP1`` / ``gpio_rp1.c``) has been
  removed. :dtcompatible:`raspberrypi,rp1-gpio` nodes are now driven by the Raspberry Pi Pico
  GPIO driver ( :kconfig:option:`CONFIG_GPIO_RPI_PICO` ), which is selected automatically when
  such a node is enabled. Out-of-tree configurations that set ``CONFIG_GPIO_RP1`` should drop it;
  the symbol no longer exists.

* The devicetree representation of :dtcompatible:`raspberrypi,rp1-gpio` nodes has changed. The
  ``reg`` property no longer packs the ``gpio``, ``rio`` and ``pads`` offsets into three address
  cells; instead each bank node carries a single ``reg`` bank index, and the register regions are
  provided by the parent :dtcompatible:`raspberrypi,rp1-pinctrl` node as ``gpioN``, ``rioN`` and
  ``padsN`` named regions. Out-of-tree devicetrees with such nodes must be updated, for example:

  .. code-block:: devicetree

     /* before */
     gpio0: gpio@1f000d0000 {
             compatible = "simple-bus";
             reg = <0x1f 0xd0000 0x30000>;

             gpio0_0: gpio@0 {
                     compatible = "raspberrypi,rp1-gpio";
                     reg = <0x0 0x10000 0x20000>;
                     /* ... */
             };
     };

     /* after */
     rp1_pinctrl: pinctrl@1f000d0000 {
             compatible = "raspberrypi,rp1-pinctrl";
             reg = <0x1f 0xd0000 0x3124>, <0x1f 0xf0004 0x3070>,
                   <0x1f 0xe0000 0x300c>;
             reg-names = "gpio0", "pads0", "rio0";

             gpio0_0: gpio@0 {
                     compatible = "raspberrypi,rp1-gpio";
                     reg = <0>;
                     /* ... */
             };
     };

* The devicetree representation of :dtcompatible:`raspberrypi,pico-gpio` nodes has changed. Now that
  these nodes are handled by the reworked :kconfig:option:`CONFIG_GPIO_RPI_PICO` driver, the ``reg``
  property must provide the ``gpio``, ``pads`` and ``sio`` register regions as named regions
  ``gpio0``, ``pads0`` and ``sio0`` instead of a single entry. The in-tree RP2040/RP2350 SoC
  devicetrees have been updated; out-of-tree devicetrees with such nodes must add the extra
  regions, for example:

  .. code-block:: devicetree

     reg = <0x40014000 DT_SIZE_K(4)>,    /* was: reg = <0x40014000 DT_SIZE_K(4)>; */
           <0x4001c000 DT_SIZE_K(4)>,
           <0xd0000000 0x180>;
     reg-names = "gpio0", "pads0", "sio0";

OPAMP
=====

* The NXP OPAMP drivers (:dtcompatible:`nxp,opamp` and :dtcompatible:`nxp,opamp-fast`) hold a
  power state constraint while the device is active, and the OPAMP nodes declare
  ``zephyr,disabling-power-states``. With :kconfig:option:`CONFIG_PM_POLICY_DEVICE_CONSTRAINTS`
  an enabled OPAMP therefore keeps the system out of Power Down and Deep Power Down for as long
  as it is active, which without :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` is from init onwards.
  Enable device runtime PM for the node, or suspend it explicitly, to let the system reach those
  states. (:github:`122135`)

* :dtcompatible:`nxp,opamp-fast` no longer enables an instance marked with
  ``is-reference-source`` from its init function; the instance follows its own power state like
  every other instance. With :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` an application has to hold
  a runtime reference on the reference source while it measures an amplifier fed by it. Builds
  without device runtime PM are unaffected: the device is resumed at init and the instance comes
  up enabled as before. (:github:`122135`)

.. zephyr-keep-sorted-stop

Bluetooth
*********

Bluetooth Classic
=================

* The L2CAP BR/EDR ECHO API is now only available with :kconfig:option:`CONFIG_BT_TESTING`:
  :c:struct:`bt_l2cap_br_echo_cb`, :c:func:`bt_l2cap_br_echo_cb_register`,
  :c:func:`bt_l2cap_br_echo_cb_unregister`, :c:func:`bt_l2cap_br_echo_req` and
  :c:func:`bt_l2cap_br_echo_rsp`. The stack answers a received ECHO request with an ECHO response
  without data, unless a registered ``req`` callback handles it.

Networking
**********

Other subsystems
****************

Modules
*******

Architectures
*************
