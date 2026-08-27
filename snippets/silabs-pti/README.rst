.. _silabs-pti:

Silicon Labs Packet Trace Interface (silabs-pti)
################################################

Overview
********

This snippet allows users to build Zephyr applications for Silicon Labs Series 2 devices
where radio packets are emitted over the Packet Trace Interface for use by debugging tools.

.. code-block:: console

   west build -S silabs-pti [...]

Requirements
************

Hardware support for :dtcompatible:`silabs,pti`.

A pinctrl configuration with nodelabel ``pti_default`` containing PTI pinout.
