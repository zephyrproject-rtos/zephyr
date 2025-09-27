.. zephyr:board:: ttgo_toiplus

Overview
********

Lilygo TTGO T-OI-PLUS is an mini IoT development board based on
Espressif's ESP32-C3 WiFi/Bluetooth dual-mode chip.

It features the following integrated components:

- ESP32-C3 SoC (RISC-V 160MHz single core, 400KB SRAM, Wi-Fi, Bluetooth)
- on board Grove connector
- USB-C connector for power and communication (on board serial)
- optional 18340 Li-ion battery holder
- LED

Functional Description
**********************
This board is based on the ESP32-C3 with 4MB of flash, WiFi and BLE support. It
has an USB-C port for programming and debugging, integrated battery charging
and an Grove connector.

Connections and IOs
===================

.. zephyr:board-supported-hw::

(Note: the above UART interface also supports connecting through USB.)

Start Application Development
*****************************

Before powering up your Lilygo TTGO T-OI-PLUS, please make sure that the board is in good
condition with no obvious signs of damage.

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

Sample Applications
*******************

The following samples will run out of the box on the TTGO T-OI-PLUS board.

To build the blinky sample:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/basic/blinky
   :board: ttgo_toiplus
   :goals: build

To build the bluetooth beacon sample:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/bluetooth/beacon
   :board: ttgo_toiplus
   :goals: build


References
**********

.. target-notes::

.. _`Lilygo TTGO T-OI-PLUS schematic`: https://github.com/Xinyuan-LilyGO/LilyGo-T-OI-PLUS/blob/main/schematic/T-OI_PLUS_Schematic.pdf
.. _`Lilygo github repo`: https://github.com/Xinyuan-LilyGO
.. _`Espressif ESP32-C3 datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf
.. _`Espressif ESP32-C3 technical reference manual`: https://www.espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
