.. zephyr:board:: maker_uno_rp2040

Overview
********

The `Cytron Maker Uno RP2040`_ board is based on the RP2040 microcontroller from Raspberry Pi Ltd.
The board has an Arduino header, several Grove connectors and a USB type C connector.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 2 Mbyte QSPI flash
- 16 GPIO pins
- 4 ADC pins
- I2C
- SPI
- UART
- USB type C connector
- LiPo charger
- Reset, boot and user buttons
- 2 RGB LEDs (Neopixels)
- Piezo buzzer with mute switch
- 4 servo ports
- Maker/Qwiic/Stemma QT/zephyr_i2c connector
- 6 Grove connectors
- Status indicators for digital pins


Default Zephyr Peripheral Mapping
=================================

The RGB LEDs are connected to GPIO25, and its pin mux setting is PIO0.

Arduino headers (note that GPIO20 and GPIO21 appear twice):

+-------+--------+-----------------+-------------------+
| Label | Pin    | Default pin mux | Also in connector |
+=======+========+=================+===================+
| A0    | GPIO26 | ADC0            | Grove 3           |
+-------+--------+-----------------+-------------------+
| A1    | GPIO27 | ADC1            | Grove 4           |
+-------+--------+-----------------+-------------------+
| A2    | GPIO28 | ADC2            | Grove 5           |
+-------+--------+-----------------+-------------------+
| A3    | GPIO29 | ADC3            | Grove 5           |
+-------+--------+-----------------+-------------------+
| SDA   | GPIO20 | I2C0 SDA        | Grove 6, Maker    |
+-------+--------+-----------------+-------------------+
| SCL   | GPIO21 | I2C0 SCL        | Grove 6, Maker    |
+-------+--------+-----------------+-------------------+
| GP1   | GPIO1  | UART0 RX        | Grove 1           |
+-------+--------+-----------------+-------------------+
| GP0   | GPIO0  | UART0 TX        | Grove 1           |
+-------+--------+-----------------+-------------------+
| GP2   | GPIO2  | GPIO pull-up    | User button       |
+-------+--------+-----------------+-------------------+
| GP3   | GPIO3  | (Alias led0)    |                   |
+-------+--------+-----------------+-------------------+
| GP4   | GPIO4  |                 | Grove 2           |
+-------+--------+-----------------+-------------------+
| GP5   | GPIO5  |                 | Grove 2           |
+-------+--------+-----------------+-------------------+
| GP6   | GPIO6  |                 | Grove 3           |
+-------+--------+-----------------+-------------------+
| GP7   | GPIO7  |                 | Grove 4           |
+-------+--------+-----------------+-------------------+
| GP8   | GPIO8  |                 | Buzzer            |
+-------+--------+-----------------+-------------------+
| GP9   | GPIO9  |                 |                   |
+-------+--------+-----------------+-------------------+
| GP13  | GPIO13 | SPI1 CS         |                   |
+-------+--------+-----------------+-------------------+
| GP11  | GPIO11 | SPI1 MOSI       | SPI header        |
+-------+--------+-----------------+-------------------+
| GP12  | GPIO12 | SPI1 MISO       | SPI header        |
+-------+--------+-----------------+-------------------+
| GP10  | GPIO10 | SPI1 SCK        | SPI header        |
+-------+--------+-----------------+-------------------+
| GP20  | GPIO20 | I2C0 SDA        | Grove 6, Maker    |
+-------+--------+-----------------+-------------------+
| GP21  | GPIO21 | I2C0 SCL        | Grove 6, Maker    |
+-------+--------+-----------------+-------------------+

SPI 6-pin header (pins also available in the Arduino header):

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| GP10  | GPIO10 | SPI1 SCK        |
+-------+--------+-----------------+
| GP11  | GPIO11 | SPI1 MOSI       |
+-------+--------+-----------------+
| GP12  | GPIO12 | SPI1 MISO       |
+-------+--------+-----------------+

Servo header:

+-------+--------+-----------------+-----------------+
| Label | Pin    | Default pin mux | Zephyr PWM name |
+=======+========+=================+=================+
| GP14  | GPIO14 | PWM7A           | 14              |
+-------+--------+-----------------+-----------------+
| GP15  | GPIO15 | PWM7B           | 15              |
+-------+--------+-----------------+-----------------+
| GP16  | GPIO16 | PWM0A           | 0               |
+-------+--------+-----------------+-----------------+
| GP17  | GPIO17 | PWM0B           | 1               |
+-------+--------+-----------------+-----------------+

Grove connector 1 (pins also available in the Arduino header):

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| GP1   | GPIO1  | UART0 RX        |
+-------+--------+-----------------+
| GP0   | GPIO0  | UART0 TX        |
+-------+--------+-----------------+

Grove connector 2 (pins also available in the Arduino header):

+-------+--------+
| Label | Pin    |
+=======+========+
| GP4   | GPIO4  |
+-------+--------+
| GP5   | GPIO5  |
+-------+--------+

Grove connector 3 (pins also available in the Arduino header):

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| GP6   | GPIO6  |                 |
+-------+--------+-----------------+
| A0    | GPIO26 | ADC0            |
+-------+--------+-----------------+

Grove connector 4 (pins also available in the Arduino header):

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| GP7   | GPIO7  |                 |
+-------+--------+-----------------+
| A1    | GPIO27 | ADC1            |
+-------+--------+-----------------+

Grove connector 5 (pins also available in the Arduino header):

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| A2    | GPIO28 | ADC2            |
+-------+--------+-----------------+
| A3    | GPIO29 | ADC3            |
+-------+--------+-----------------+

Grove connector 6 (pins also available in the Maker connector and Arduino header):

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| GP20  | GPIO20 | I2C0 SDA        |
+-------+--------+-----------------+
| GP21  | GPIO21 | I2C0 SCL        |
+-------+--------+-----------------+

Maker connector, also known as Qwiic/Stemma QT/zephyr_i2c. The pins are also
available in Grove 6 and the Arduino header:

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| GP20  | GPIO20 | I2C0 SDA        |
+-------+--------+-----------------+
| GP21  | GPIO21 | I2C0 SCL        |
+-------+--------+-----------------+

See also `schematic`_.


Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The Maker Uno RP2040 board does not expose the SWDIO and SWCLK pins, so programming must be
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
   :board: maker_uno_rp2040
   :goals: build flash

Try also the :zephyr:code-sample:`hello_world`, :zephyr:code-sample:`blinky`,
:zephyr:code-sample:`button`, :zephyr:code-sample:`input-dump` and
:zephyr:code-sample:`adc_dt` samples.

The use of the Maker/Qwiic/Stemma QT I2C connector is demonstrated using the
:zephyr:code-sample:`light_sensor_polling` sample and a separate shield:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/light_polling
   :board: maker_uno_rp2040
   :shield: adafruit_veml7700
   :goals: build flash

Use the shell to control the GPIO pins:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: maker_uno_rp2040
   :gen-args: -DCONFIG_GPIO=y -DCONFIG_GPIO_SHELL=y
   :goals: build flash

To set one of the GPIO pins high, use these commands in the shell, and study the indicator LEDs:

.. code-block:: shell

    gpio conf gpio0 2 o
    gpio set gpio0 2 1

Servo motor control is done via PWM outputs. The :zephyr:code-sample:`servo-motor`
sample sets servo position timing (via an overlay file) for the output GP14:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/servo_motor/
   :board: maker_uno_rp2040
   :goals: build flash

It is also possible to control servos via the pwm shell:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: maker_uno_rp2040
   :gen-args: -DCONFIG_PWM=y -DCONFIG_PWM_SHELL=y
   :goals: build flash

Use shell commands to set the posiotion of the server. Most servo motor can handle pulse
times between 800 and 2000 microseconds:

.. code-block:: shell

    pwm usec pwm@40050000 14 20000 800
    pwm usec pwm@40050000 14 20000 2000

To use the buzzer, you must set the pin mux for GPIO8 to PWM. This is done by adding ``PWM_4A_P8``
to the ``pwm_default`` section in the
:zephyr_file:`boards/cytron/maker_uno_rp2040/maker_uno_rp2040-pinctrl.dtsi` file.
Turn on the buzzer switch on the long side of the board. Then build using the same command
as above for the sensor_shell.

Use these shell commands to turn on and off the buzzer:

.. code-block:: shell

    pwm usec pwm@40050000 8 1000 500
    pwm usec pwm@40050000 8 1000 0


References
**********

.. target-notes::

.. _Cytron Maker Uno RP2040:
    https://www.cytron.io/p-maker-uno-rp2040

.. _schematic:
    https://drive.google.com/file/d/1BNqbxXScMXnL3-2YYfbR66nFCD1li71X/view
