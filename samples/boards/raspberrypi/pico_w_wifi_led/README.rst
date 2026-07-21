.. zephyr:code-sample:: pico-w-wifi-led
   :name: Pico W WiFi + LED
   :relevant-api: gpio_interface

   Blink the onboard LED while scanning for WiFi networks.

Overview
********

This sample demonstrates concurrent WiFi and LED operation on the
Raspberry Pi Pico W and Pico 2 W. A background thread blinks the
onboard LED via the CYW43 GPIO driver while the main thread performs
a WiFi scan.

Requirements
************

This sample requires a board with an Infineon CYW43439 WiFi chip:

- Raspberry Pi Pico W (``rpi_pico/rp2040/w``)
- Raspberry Pi Pico 2 W (``rpi_pico2/rp2350a/m33/w``)

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/raspberrypi/pico_w_wifi_led
   :board: rpi_pico2/rp2350a/m33/w
   :goals: build flash

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0 ***
   Pico W WiFi + LED Demo
   LED blinking started
   Starting WiFi scan...
     SSID: MyNetwork              CH:  6 RSSI: -45
   WiFi scan complete
   Demo complete - LED continues blinking
