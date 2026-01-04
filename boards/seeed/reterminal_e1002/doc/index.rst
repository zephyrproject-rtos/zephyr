.. zephyr:board:: reterminal_e1002

Overview
********

Seeed Studio reTerminal E1002 is a board with a 7.3 inch full color ePaper display with
a built-in 2000 mAh battery based on the Espressif ESP32-S3 WiFi/Bluetooth dual-mode chip.

For more details see the `Seeed Studio reTerminal E1002`_ wiki page.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The board uses a 8-pin expansion header with the following pin mapping:

.. list-table:: Expansion header
   :header-rows: 1

   * - Pin
     - Label
     - ESP32-S3 Pin
     - Function
     - Description
   * - 1
     - HEADER_3V3
     - -
     - Power
     - 3.3V power supply for external devices
   * - 2
     - GND
     - -
     - Ground
     - Common ground reference
   * - 3
     - ESP_IO46
     - GPIO46
     - GPIO/ADC
     - General purpose I/O with analog input capability
   * - 4
     - ESP_IO2/ADC1_CH4
     - GPIO2
     - GPIO/ADC
     - General purpose I/O with analog input capability
   * - 5
     - ESP_IO17/TX1
     - GPIO17
     - GPIO/UART TX
     - GPIO or UART transmit (TX) signal
   * - 6
     - ESP_IO18/RX1
     - GPIO18
     - GPIO/UART RX
     - GPIO or UART receive (RX) signal
   * - 7
     - ESP_IO20/I2C0_SCL
     - GPIO20
     - GPIO/I2C SCL
     - GPIO or I2C clock signal
   * - 8
     - ESP_IO19/I2C0_SDA
     - GPIO19
     - GPIO/I2C SDA
     - GPIO or I2C data signal

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

Backup the original firmware
============================

The following command can be used to backup the original firmware:

.. code-block:: shell

   esptool -c esp32s3 -p /dev/ttyUSB0 read-flash 0x0 0x2000000 fw-backup-32MB.bin

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`Seeed Studio reTerminal E1002`: https://wiki.seeedstudio.com/getting_started_with_reterminal_e1002/
