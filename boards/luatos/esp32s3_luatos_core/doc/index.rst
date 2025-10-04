.. zephyr:board:: esp32s3_luatos_core

Overview
********

The ESP32S3-Luatos-Core development board is a compact board based on Espressif ESP32-S3.
The board comes equipped with a 2.4GHz antenna and supports both Wi-Fi and Bluetooth functionalities.
For more information, check `ESP32S3-Luatos-Core`_ (chinese)

.. image:: img/esp32s3_luatos_core.jpg
     :align: center
     :alt: esp32s3_luatos_core

Hardware
********

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

.. image:: img/esp32s3_luatos_core_pinout.jpg
     :align: center
     :alt: esp32s3_luatos_core_pinout

Supported Features
==================

.. zephyr:board-supported-hw::

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

.. _`ESP32S3-Luatos-Core`: https://wiki.luatos.com/chips/esp32s3/board.html
.. _`ESP32-S3 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-s3-mini-1_mini-1u_datasheet_en.pdf
.. _`ESP32-S3 Technical Reference Manual`: https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf
.. _`JTAG debugging for ESP32-S3`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/jtag-debugging/
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
