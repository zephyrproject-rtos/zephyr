.. _esp32s2_lolin_mini:

ESP32-S2 Lolin Mini
###################

Overview
********

ESP32-S2 is a highly integrated, low-power, single-core Wi-Fi Microcontroller SoC, designed to be secure and
cost-effective, with a high performance and a rich set of IO capabilities. [1]_

The features include the following:

- RSA-3072-based secure boot
- AES-XTS-256-based flash encryption
- Protected private key and device secrets from software access
- Cryptographic accelerators for enhanced performance
- Protection against physical fault injection attacks
- Various peripherals:

  - 43x programmable GPIOs
  - 14x configurable capacitive touch GPIOs
  - USB OTG
  - LCD interface
  - camera interface
  - SPI
  - I2S
  - UART
  - ADC
  - DAC
  - LED PWM with up to 8 channels

.. figure:: img/esp32_s2_lolin_mini.jpg
        :align: center
        :alt: ESP32-S2 LOLIN MINI

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
   :board: esp32s2_lolin_mini
   :goals: build

The usual ``flash`` target will work with the ``esp32s2_lolin_mini`` board
configuration after putting the board into bootloader mode by holding the '0'
button then pressing 'RST' and releasing the 'RST' button.

Here is an example for the :ref:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32s2_lolin_mini
   :goals: flash

Perform the needed connections (see: Helper hardware connections section)
to see :ref:`hello_world` output on the screen and open a serial port using e.g. screen:

.. code-block:: shell

   screen /dev/ttyUSB0 115200

Or using espressif monitor

.. code-block:: shell

   west espressif monitor


After the board has been manually reset and booted, you should see the following
message in the monitor (see: Hardware Connections section)

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! esp32s2_lolin_mini

Helper Hardware connections
---------------------------

.. warning::

   To this date esp32s2 SoC does not possess USB CDC-ACM support in zephyr.
   By default zephyr logs are redirected to UART0 through pins 16 and 17)

The following image depicts generic connections from Wemos ESP32-S2 Lolin Mini
UART0 port to a UART-TTL converter to allow log monitoring:

.. figure:: img/wemos_esp32s2_mini_uart_connection.png
        :align: center
        :alt: ESP32-S2 LOLIN MINI UART CONNECTION


References
**********

.. [1] https://www.espressif.com/en/products/socs/esp32-s2
.. _`ESP32S2 Technical Reference Manual`: https://espressif.com/sites/default/files/documentation/esp32-s2_technical_reference_manual_en.pdf
.. _`ESP32S2 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-s2_datasheet_en.pdf
