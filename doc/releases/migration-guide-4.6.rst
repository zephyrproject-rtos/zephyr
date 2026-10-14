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

.. zephyr-keep-sorted-stop

Bluetooth
*********

Haptics
*******

* Vendor prefixes have been added to hardware-specific devicetree properties. Applications using the
  following devicetree properties must update accordingly:

  * :dtcompatible:`cirrus,cs40l26`, :dtcompatible:`cirrus,cs40l27`, :dtcompatible:`cirrus,cs40l50`,
    :dtcompatible:`cirrus,cs40l51`, :dtcompatible:`cirrus,cs40l52`, :dtcompatible:`cirrus,cs40l53`

    * ``trigger-mapping`` changed to ``cirrus,trigger-mapping``
    * ``flash-storage`` changed to ``cirrus,flash-storage``
    * ``flash-offset`` changed to ``cirrus,flash-offset``

  * :dtcompatible:`ti,drv2605`

    * ``actuator-mode`` changed to ``ti,actuator-mode``
    * ``feedback-brake-factor`` changed to ``ti,feedback-brake-factor``
    * ``loop-gain`` changed to ``ti,loop-gain``
    * ``vib-rated-mv`` changed to ``ti,vib-rated-mv``
    * ``vib-overdrive-mv`` changed to ``ti,vib-overdrive-mv``

  * :dtcompatible:`titanmec,tm6605`

    * ``default-effect`` changed to ``titanmec,default-effect``

Networking
**********

Other subsystems
****************

Modules
*******

Architectures
*************
