.. zephyr:board:: glyph_h2

Overview
********

Glyph-H2 is an entry-level development board based on the ESP32-H2-MINI-1 module,
which integrates Bluetooth® Low Energy (LE) and IEEE 802.15.4 connectivity. It features
the ESP32-H2 SoC — a 32-bit RISC-V core designed for low-power, secure wireless communication,
supporting Bluetooth 5 (LE), Bluetooth Mesh, Thread, Matter, and Zigbee protocols.
This module is ideal for a wide range of low-power IoT applications.

For details on getting started, check `Glyph-H2`_.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32h2-features.rst
   :start-after: espressif-soc-esp32h2-features

Supported Features
==================

.. zephyr:board-supported-hw::

System Requirements
*******************

Espressif HAL requires Bluetooth binary blobs in order work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

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

.. _`Glyph-H2`: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32h2/esp32-h2-devkitm-1/user_guide.html
.. _`ESP32-H2 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-h2_datasheet_en.pdf
.. _`ESP32-H2 Technical Reference Manual`: https://www.espressif.com/sites/default/files/documentation/esp32-h2_technical_reference_manual_en.pdf
