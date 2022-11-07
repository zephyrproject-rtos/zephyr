.. _heltec_wifi_lora32_v2:

Heltec WiFi LoRa 32 (V2)
########################

Overview
********

Heltec WiFi LoRa 32 is a classic IoT dev-board designed & produced by Heltec Automation(TM), it's a highly
integrated product based on ESP32 + SX127x, it has Wi-Fi, BLE, LoRa functions, also Li-Po battery management
system, 0.96" OLED are also included. [1]_

The features include the following:

- Microprocessor: ESP32 (dual-core 32-bit MCU + ULP core)
- LoRa node chip SX1276/SX1278
- Micro USB interface with a complete voltage regulator, ESD protection, short circuit protection,
  RF shielding, and other protection measures
- Onboard SH1.25-2 battery interface, integrated lithium battery management system
- Integrated WiFi, LoRa, Bluetooth three network connections, onboard Wi-Fi, Bluetooth dedicated 2.4GHz
  metal 3D antenna, reserved IPEX (U.FL) interface for LoRa use
- Onboard 0.96-inch 128*64 dot matrix OLED display
- Integrated CP2102 USB to serial port chip

System requirements
*******************

Prerequisites
-------------

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

Building & Flashing
-------------------

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: heltec_wifi_lora32_v2
   :goals: build

The usual ``flash`` target will work with the ``heltec_wifi_lora32_v2`` board
configuration. Here is an example for the :ref:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: heltec_wifi_lora32_v2
   :goals: flash

Open the serial monitor using the following command:

.. code-block:: shell

   west espressif monitor

After the board has automatically reset and booted, you should see the following
message in the monitor:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! heltec_wifi_lora32_v2

Debugging
---------

As with much custom hardware, the ESP32 modules require patches to
OpenOCD that are not upstreamed yet. Espressif maintains their own fork of
the project. The custom OpenOCD can be obtained at `OpenOCD ESP32`_

The Zephyr SDK uses a bundled version of OpenOCD by default. You can overwrite that behavior by adding the
``-DOPENOCD=<path/to/bin/openocd> -DOPENOCD_DEFAULT_PATH=<path/to/openocd/share/openocd/scripts>``
parameter when building.

Here is an example for building the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: heltec_wifi_lora32_v2
   :goals: build flash
   :gen-args: -DOPENOCD=<path/to/bin/openocd> -DOPENOCD_DEFAULT_PATH=<path/to/openocd/share/openocd/scripts>

You can debug an application in the usual way. Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: heltec_wifi_lora32_v2
   :goals: debug

Utilizing Hardware Features
***************************

Onboard OLED display
--------------------

The onboard OLED display is of type ``ssd1306``, has 128*64 pixels and is
connected via I2C. It can therefore be used by enabling the
:ref:`ssd1306_128_shield` as shown in the following for the :ref:`lvgl-sample`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: heltec_wifi_lora32_v2
   :shield: ssd1306_128x64
   :goals: flash

References
**********

- `Heltec WiFi LoRa (v2) Pinout Diagram <https://resource.heltec.cn/download/WiFi_LoRa_32/WIFI_LoRa_32_V2.pdf>`_
- `Heltec WiFi LoRa (v2) Schematic Diagrams <https://resource.heltec.cn/download/WiFi_LoRa_32/V2>`_
- `ESP32 Toolchain <https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-guides/tools/idf-tools.html#xtensa-esp32-elf>`_
- `esptool documentation <https://github.com/espressif/esptool/blob/master/README.md>`_
- `OpenOCD ESP32 <https://github.com/espressif/openocd-esp32/releases>`_

.. [1] https://heltec.org/project/wifi-lora-32/
