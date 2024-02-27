:orphan:

.. _migration_3.7:

Migration guide to Zephyr v3.7.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v3.6.0 to
Zephyr v3.7.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_3.7>`.

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

MCUboot
=======

zcbor
=====

Device Drivers and Devicetree
*****************************

Analog-to-Digital Converter (ADC)
=================================

Bluetooth HCI
=============

Controller Area Network (CAN)
=============================

Display
=======

Flash
=====

General Purpose I/O (GPIO)
==========================

Input
=====

Interrupt Controller
====================

Sensors
=======

Serial
======

Timer
=====

Bluetooth
*********

Bluetooth Mesh
==============

Bluetooth Audio
===============

Networking
**********

* The zperf zperf_results struct is changed to support 64 bits transferred bytes (total_len)
  and test duration (time_in_us and client_time_in_us), instead of 32 bits. This will make
  the long-duration zperf test show with correct throughput result.
  (:github:`69500`)

Other Subsystems
****************

LoRaWAN
=======

MCUmgr
======

Shell
=====

ZBus
====

Userspace
*********

Architectures
*************

Xtensa
======
