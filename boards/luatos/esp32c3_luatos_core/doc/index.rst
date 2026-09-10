.. zephyr:board:: esp32c3_luatos_core

Overview
********

ESP32-C3 is a single-core Wi-Fi and Bluetooth 5 (LE) microcontroller SoC,
based on the open-source RISC-V architecture. It strikes the right balance of power,
I/O capabilities and security, thus offering the optimal cost-effective
solution for connected devices.
The availability of Wi-Fi and Bluetooth 5 (LE) connectivity not only makes the device configuration easy,
but it also facilitates a variety of use-cases based on dual connectivity.
See the `ESP32C3 Luatos Core Website`_ for more details.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32c3-features.rst
   :start-after: espressif-soc-esp32c3-features

There are two version hardware of this board. The difference between them is the ch343 chip.

1. USB-C connect to UART over CH343 chip(esp32c3_luatos_core)

.. image:: img/esp32c3_luatos_core.jpg
     :align: center
     :alt: esp32c3_luatos_core

2. USB-C connect to esp32 chip directly(esp32c3_luatos_core/esp32c3/usb)

.. image:: img/esp32c3_luatos_core_usb.jpg
     :align: center
     :alt: esp32c3_luatos_core/esp32c3/usb

Supported Features
==================

.. zephyr:board-supported-hw::

Connection and IO
=================

.. image:: img/esp32c3_luatos_core_pinfunc.jpg
     :align: center
     :alt: esp32c3_luatos_core_pinfunc

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`ESP32C3 Luatos Core Website`: https://wiki.luatos.com/chips/esp32c3/board.html
