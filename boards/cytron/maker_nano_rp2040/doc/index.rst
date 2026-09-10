.. zephyr:board:: maker_nano_rp2040

Overview
********

The `Cytron Maker Nano RP2040`_ board is based on the RP2040 microcontroller from Raspberry Pi Ltd.
The board has an Arduino Nano header, Maker/Qwiic/Stemma QT connectors and a mikro USB connector.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 2 Mbyte QSPI flash
- 20 GPIO pins
- 2 ADC pins
- I2C
- SPI
- UART
- Mikro USB connector
- Reset, boot and user buttons
- 2 RGB LEDs (Neopixels)
- Piezo buzzer with mute switch
- Maker/Qwiic/Stemma QT/zephyr_i2c connectors
- Status indicators for digital pins


Default Zephyr Peripheral Mapping
=================================

+-------------+--------+-----------------+-------------------+
| Label       | Pin    | Default pin mux | Notes             |
+=============+========+=================+===================+
| RGB LEDs    | GPIO11 | PIO0            |                   |
+-------------+--------+-----------------+-------------------+
| User button | GPIO20 |                 | Alias sw0         |
+-------------+--------+-----------------+-------------------+
| Buzzer      | GPIO22 | PWM3A           | Zephyr PWM name 6 |
+-------------+--------+-----------------+-------------------+


Arduino Nano header:

+-------+--------+-----------------+-------------------+
| Label | Pin    | Default pin mux | Also in connector |
+=======+========+=================+===================+
| 0     | GPIO0  | UART0 TX        | Maker port 0      |
+-------+--------+-----------------+-------------------+
| 1     | GPIO1  | UART0 RX        | Maker port 0      |
+-------+--------+-----------------+-------------------+
| RS    | Reset  |                 |                   |
+-------+--------+-----------------+-------------------+
| G     | GND    |                 |                   |
+-------+--------+-----------------+-------------------+
| 2     | GPIO2  |                 | (Alias led0)      |
+-------+--------+-----------------+-------------------+
| 3     | GPIO3  |                 |                   |
+-------+--------+-----------------+-------------------+
| 4     | GPIO4  |                 |                   |
+-------+--------+-----------------+-------------------+
| 5     | GPIO5  |                 |                   |
+-------+--------+-----------------+-------------------+
| 6     | GPIO6  |                 |                   |
+-------+--------+-----------------+-------------------+
| 7     | GPIO7  |                 |                   |
+-------+--------+-----------------+-------------------+
| 8     | GPIO8  |                 |                   |
+-------+--------+-----------------+-------------------+
| 9     | GPIO9  |                 |                   |
+-------+--------+-----------------+-------------------+
| 17    | GPIO17 | SPI0 CS         |                   |
+-------+--------+-----------------+-------------------+
| 19    | GPIO19 | SPI0 MOSI       |                   |
+-------+--------+-----------------+-------------------+
| 16    | GPIO16 | SPI0 MISO       |                   |
+-------+--------+-----------------+-------------------+
| 18    | GPIO18 | SPI0 SCK        |                   |
+-------+--------+-----------------+-------------------+
| 3V3   | 3.3 V  |                 |                   |
+-------+--------+-----------------+-------------------+
| NC    |        |                 |                   |
+-------+--------+-----------------+-------------------+
| 26    | GPIO26 | I2C1 SDA        | Maker port 1      |
+-------+--------+-----------------+-------------------+
| 27    | GPIO27 | I2C1 SCL        | Maker port 1      |
+-------+--------+-----------------+-------------------+
| 28    | GPIO28 | ADC2            |                   |
+-------+--------+-----------------+-------------------+
| 29    | GPIO29 | ADC3            |                   |
+-------+--------+-----------------+-------------------+
| 12    | GPIO12 | I2C0 SDA        |                   |
+-------+--------+-----------------+-------------------+
| 13    | GPIO13 | I2C0 SCL        |                   |
+-------+--------+-----------------+-------------------+
| 14    | GPIO14 |                 |                   |
+-------+--------+-----------------+-------------------+
| 15    | GPIO15 |                 |                   |
+-------+--------+-----------------+-------------------+
| 5V    | 5 V    |                 |                   |
+-------+--------+-----------------+-------------------+
| RS    | Reset  |                 |                   |
+-------+--------+-----------------+-------------------+
| G     | GND    |                 |                   |
+-------+--------+-----------------+-------------------+
| VIN   | Vin    |                 |                   |
+-------+--------+-----------------+-------------------+


Maker port 0 (pins also available in the Arduino Nano header):

+--------+-----------------+
| Pin    | Default pin mux |
+========+=================+
| GPIO0  | UART0 TX        |
+--------+-----------------+
| GPIO1  | UART0 RX        |
+--------+-----------------+


Maker port 1, also known as Qwiic/Stemma QT/zephyr_i2c. Pins also available in the Arduino
Nano header:

+--------+-----------------+
| Pin    | Default pin mux |
+========+=================+
| GPIO26 | I2C1 SDA        |
+--------+-----------------+
| GPIO27 | I2C1 SCL        |
+--------+-----------------+

See also `pinout`_.


Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The Maker Nano RP2040 board does not expose the SWDIO and SWCLK pins, so programming must be
done via the USB port. Press and hold the BOOT button, and then press the RST button,
and the device will appear as a USB mass storage unit.
Building your application will result in a :file:`build/zephyr/zephyr.uf2` file.
Drag and drop the file to the USB mass storage unit, and the board will be reprogrammed.

For more details on programming RP2040-based boards, see :ref:`rpi_pico_programming_and_debugging`.


Flashing
========

To run the :zephyr:code-sample:`led-strip` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/led_strip/
   :board: maker_nano_rp2040
   :goals: build flash

Try also the :zephyr:code-sample:`hello_world`, :zephyr:code-sample:`blinky`,
:zephyr:code-sample:`button`, :zephyr:code-sample:`input-dump` and
:zephyr:code-sample:`adc_dt` samples.

The use of the Maker Port 1/Qwiic/Stemma QT I2C connector is demonstrated using the
:zephyr:code-sample:`light_sensor_polling` sample and a separate shield:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/light_polling
   :board: maker_nano_rp2040
   :shield: adafruit_veml7700
   :goals: build flash

Use the shell to control the GPIO pins:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: maker_nano_rp2040
   :gen-args: -DCONFIG_GPIO=y -DCONFIG_GPIO_SHELL=y
   :goals: build flash

To set one of the GPIO pins high, use these commands in the shell, and study the indicator LEDs:

.. code-block:: shell

    gpio conf gpio0 2 o
    gpio set gpio0 2 1

Turn on the buzzer switch on the short side of the board. Then build using the same command
as above for the sensor_shell. Use these shell commands to turn on and off the buzzer:

.. code-block:: shell

    pwm usec pwm@40050000 6 1000 500
    pwm usec pwm@40050000 6 1000 0


References
**********

.. target-notes::

.. _Cytron Maker Nano RP2040:
    https://www.cytron.io/c-maker-series/p-maker-nano-rp2040-simplifying-projects-with-raspberry-pi-rp2040

.. _pinout:
    https://docs.google.com/drawings/d/e/2PACX-1vSGwfh_1ac_UFXT4F72D0yJHaYHjDC-lfeBMLp0dc8ry57sAYtdobIFBZqrfXE6AuDTYEY9Cicto2b8/pub?w=3373&h=2867
