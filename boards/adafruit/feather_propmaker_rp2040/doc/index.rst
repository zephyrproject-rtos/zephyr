.. zephyr:board:: adafruit_feather_propmaker_rp2040

Overview
********

The `Adafruit RP2040 Prop-Maker Feather`_ board is based on the RP2040
microcontroller from Raspberry Pi Ltd.
It has an output for external RGB LEDs (Neopixels), an external button input and an I2S-based
speaker output. There is a Stemma QT connector for easy sensor usage.
It is compatible with the Feather board form factor, and has a USB type C connector.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 8 Mbyte QSPI flash
- 18 GPIO pins
- 4 ADC pins
- I2C
- SPI
- UART
- USB type C connector
- Reset and boot buttons
- On-board Red LED
- On-board RGB LED (Neopixel)
- Stemma QT I2C connector (also known as Qwiic)
- LIS3DH accelerometer
- Servo port
- Built-in lithium ion battery charger
- I2S amplifier and speaker output
- External button input
- External neopixel output


Default Zephyr Peripheral Mapping
=================================

+------------------+--------+-----------------+-------------------+
| Description      | Pin    | Default pin mux | Comment           |
+==================+========+=================+===================+
| Boot button      | GPIO7  |                 | Alias sw0         |
+------------------+--------+-----------------+-------------------+
| I2S DIN          | GPIO16 |                 |                   |
+------------------+--------+-----------------+-------------------+
| I2S BCLK         | GPIO17 |                 |                   |
+------------------+--------+-----------------+-------------------+
| I2S LRCLK        | GPIO18 |                 |                   |
+------------------+--------+-----------------+-------------------+
| Servo output     | GPIO20 | PWM_2A          | PWM name 4        |
+------------------+--------+-----------------+-------------------+
| Accel. interrupt | GPIO22 |                 |                   |
+------------------+--------+-----------------+-------------------+


Feather header:

+-------+--------+-----------------+-------------------+
| Label | Pin    | Default pin mux | Also used by      |
+=======+========+=================+===================+
| A0    | GPIO26 | ADC0            |                   |
+-------+--------+-----------------+-------------------+
| A1    | GPIO27 | ADC1            |                   |
+-------+--------+-----------------+-------------------+
| A2    | GPIO28 | ADC2            |                   |
+-------+--------+-----------------+-------------------+
| A3    | GPIO29 | ADC3            |                   |
+-------+--------+-----------------+-------------------+
| 24    | GPIO24 |                 |                   |
+-------+--------+-----------------+-------------------+
| 25    | GPIO25 |                 |                   |
+-------+--------+-----------------+-------------------+
| SCK   | GPIO14 | SPI1 SCK        |                   |
+-------+--------+-----------------+-------------------+
| MO    | GPIO15 | SPI1 MOSI       |                   |
+-------+--------+-----------------+-------------------+
| MI    | GPIO8  | SPI1 MISO       |                   |
+-------+--------+-----------------+-------------------+
| RX    | GPIO1  | UART0 RX        |                   |
+-------+--------+-----------------+-------------------+
| TX    | GPIO0  | UART0 TX        |                   |
+-------+--------+-----------------+-------------------+
| 4     | GPIO4  | PIO0            | On-board RGB LED  |
+-------+--------+-----------------+-------------------+
| SDA   | GPIO2  | I2C1 SDA        | Stemma QT         |
+-------+--------+-----------------+-------------------+
| SCL   | GPIO3  | I2C1 SCL        | Stemma QT         |
+-------+--------+-----------------+-------------------+
| 5     | GPIO5  |                 |                   |
+-------+--------+-----------------+-------------------+
| 6     | GPIO6  |                 |                   |
+-------+--------+-----------------+-------------------+
| 9     | GPIO9  |                 |                   |
+-------+--------+-----------------+-------------------+
| 10    | GPIO10 |                 |                   |
+-------+--------+-----------------+-------------------+
| 11    | GPIO11 |                 |                   |
+-------+--------+-----------------+-------------------+
| 12    | GPIO12 |                 |                   |
+-------+--------+-----------------+-------------------+
| 13    | GPIO13 |                 | Red user LED      |
+-------+--------+-----------------+-------------------+


Stemma QT I2C connector (pins also available in the Feather header):

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| SDA   | GPIO2  | I2C1 SDA        |
+-------+--------+-----------------+
| SCL   | GPIO3  | I2C1 SCL        |
+-------+--------+-----------------+

The LIS3DH accelerometer is connected to the I2C bus, and has the 7-bit address 0x18.


Output screw terminal:

+-----+-------------------------------------------+-------------------------+
| Pin | Description                               | Default pin mux         |
+=====+===========================================+=========================+
| 1   | RGB LED from GPIO21                       | PIO1                    |
+-----+-------------------------------------------+-------------------------+
| 2   | GND                                       |                         |
+-----+-------------------------------------------+-------------------------+
| 3   | +5 Volt power output controlled by GPIO23 | gpio-hog always enabled |
+-----+-------------------------------------------+-------------------------+
| 4   | Button input to GPIO19                    |                         |
+-----+-------------------------------------------+-------------------------+
| 5   | Speaker -                                 |                         |
+-----+-------------------------------------------+-------------------------+
| 6   | Speaker +                                 |                         |
+-----+-------------------------------------------+-------------------------+

The +5V output is always enabled via a ``gpio-hog`` in the devicetree file. Remove the node
``servo-power-enable`` to avoid powering the output on startup.

See also `pinout`_ and `schematic`_.


Supported Features
==================

Note that the I2S-based speaker output is not yet supported.

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The Adafruit RP2040 Prop-Maker Feather board does not expose the SWDIO and SWCLK pins, so
programming must be done via the USB port. Press and hold the BOOT button, and then press
the RST button, and the device will appear as a USB mass storage unit.
Building your application will result in a :file:`build/zephyr/zephyr.uf2` file.
Drag and drop the file to the USB mass storage unit, and the board will be reprogrammed.

For more details on programming RP2040-based boards, see :ref:`rpi_pico_programming_and_debugging`.


Flashing
========

To run the :zephyr:code-sample:`blinky` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky/
   :board: adafruit_feather_propmaker_rp2040
   :goals: build flash

Try also the :zephyr:code-sample:`led-strip`, :zephyr:code-sample:`input-dump`,
:zephyr:code-sample:`button`, :zephyr:code-sample:`accel_polling`,
:zephyr:code-sample:`hello_world` and :zephyr:code-sample:`adc_dt` samples.

The use of the Stemma QT I2C connector is demonstrated using the
:zephyr:code-sample:`light_sensor_polling` sample and a separate shield:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/light_polling/
   :board: adafruit_feather_propmaker_rp2040
   :shield: adafruit_veml7700
   :goals: build flash

Servo motor control is done via PWM outputs. The :zephyr:code-sample:`servo-motor` sample sets
servo position timing via an overlay file.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/servo_motor/
   :board: adafruit_feather_propmaker_rp2040
   :goals: build flash

It is also possible to control the servo via the PWM shell:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell/
   :board: adafruit_feather_propmaker_rp2040
   :gen-args: -DCONFIG_PWM=y -DCONFIG_PWM_SHELL=y
   :goals: build flash

Control the position via the pulse length. Most servos can handle pulse lengths between 0.8
and 2 milliseconds:

.. code-block:: shell

    pwm usec pwm@40050000 4 20000 800
    pwm usec pwm@40050000 4 20000 2000

In order to use external neopixels connected via the screw terminal, you need to adapt the
devicetree file to the number of pixels in your external light-strip. See the ``external_ws2812``
node. You must also change the ``aliases`` attribute from ``led-strip = &onboard_ws2812;`` to
``led-strip = &external_ws2812;``. Then use the :zephyr:code-sample:`led-strip` sample.

Read the external button via GPIO.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell/
   :board: adafruit_feather_propmaker_rp2040
   :gen-args: -DCONFIG_GPIO_SHELL=y
   :goals: build flash

Use these shell commands to read the external button:

.. code-block:: shell

    gpio conf gpio0 19 iul
    gpio get gpio0 19

If you wish to map the external button to the Zephyr input subsystem, see how it is done for the
on-board button in the devicetree file.


References
**********

.. target-notes::

.. _Adafruit RP2040 Prop-Maker Feather:
   https://learn.adafruit.com/adafruit-rp2040-prop-maker-feather

.. _pinout:
   https://learn.adafruit.com/adafruit-rp2040-prop-maker-feather/pinouts

.. _schematic:
    https://learn.adafruit.com/adafruit-rp2040-prop-maker-feather/downloads-2
