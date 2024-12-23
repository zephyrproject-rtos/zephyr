.. _esp_tz_router:

Overview
********

The ESP Thread Border Router / ZigBee router developer platform.

Hardware
********

TODO: merge PR

Asymmetric Multiprocessing (AMP)
********************************

ESP32S3-DevKitC allows 2 different applications to be executed in ESP32-S3 SoC. Due to its dual-core
architecture, each core can be enabled to execute customized tasks in stand-alone mode
and/or exchanging data over OpenAMP framework. See :ref:`ipc_samples` folder as code reference.

For more information, check the datasheet at `ESP32-S3 Datasheet`_.

Supported Features
==================

.. zephyr:board-supported-hw::

System Requirements
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

.. _`JTAG debugging for ESP32-S3`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/jtag-debugging/
.. _`OpenOCD`: https://github.com/openocd-org/openocd
.. _`ESP32-S3 DevKitC`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html

References
**********

.. target-notes::

.. _`ESP32-S3 DevKitC User Guide`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html
.. _`ESP32-S3 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf
.. _`ESP32 Technical Reference Manual`: https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf
