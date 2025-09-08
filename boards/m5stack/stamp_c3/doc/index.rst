.. zephyr:board:: stamp_c3

Overview
********

STAMP-C3 featuring ESPRESSIF ESP32-C3 RISC-V MCU with Wi-Fi connectivity
for IoT edge devices such as home appliances and Industrial Automation.

For more details see the `M5Stack STAMP-C3`_ page.

Supported Features
==================

.. zephyr:board-supported-hw::

System requirements
*******************

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the command
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

.. _`M5Stack STAMP-C3`: https://docs.m5stack.com/en/core/stamp_c3
.. _`ESP32C3 Technical Reference Manual`: https://espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf
.. _`ESP32C3 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
