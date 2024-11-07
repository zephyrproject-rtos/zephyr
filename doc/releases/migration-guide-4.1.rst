:orphan:

.. _migration_4.1:

Migration guide to Zephyr v4.1.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.0.0 to
Zephyr v4.1.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.1>`.

.. contents::
    :local:
    :depth: 2

Build System
************

Kernel
******

Boards
******

Modules
*******

Mbed TLS
========

Trusted Firmware-M
==================

LVGL
====

Device Drivers and Devicetree
*****************************

* The :dtcompatible: ``nxp,lpc-iocon`` and ``nxp,rt-iocon-pinctrl`` driver won't be used
  for RT 3 digital platforms.
  New :dtcompatible:`nxp,iopctl` and :dtcompatible:`nxp,rt-iopctl-pinctrl` have been created
  for iopctl IP on RT 3 digital platforms. Change iocon node to iopctl0 node on RT500/600
  platforms. New pinctrl model add instance index parameter in pin header files, however,
  for the application layer, the pin macro name will not change. So it means application
  layer won't be affected by changes in the driver layer.(:github:`81086`)
  example:

  .. code-block:: devicetree

    / {
        iopctl0: iopctl@4000 {
            compatible = "nxp,iopctl";
            reg = <0x4000 0x1000>;
            status = "okay";
            pinctrl: pinctrl {
                compatible = "nxp,rt-iopctl-pinctrl";
            };
    };

Controller Area Network (CAN)
=============================

Display
=======

Enhanced Serial Peripheral Interface (eSPI)
===========================================

GNSS
====

Input
=====

Interrupt Controller
====================

LED Strip
=========

Sensors
=======

Serial
======

Regulator
=========

Bluetooth
*********

Bluetooth HCI
=============

Bluetooth Mesh
==============

Bluetooth Audio
===============

Bluetooth Classic
=================

Bluetooth Host
==============

Bluetooth Crypto
================

Networking
**********

Other Subsystems
****************

Flash map
=========

hawkBit
=======

MCUmgr
======

Modem
=====

Architectures
*************
