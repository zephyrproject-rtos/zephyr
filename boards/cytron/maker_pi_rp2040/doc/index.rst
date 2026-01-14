.. zephyr:board:: maker_pi_rp2040

Overview
********

The `Cytron Maker Pi RP2040`_ board is based on the RP2040 microcontroller from Raspberry Pi Ltd.
The board has motor drivers, servo headers, Grove connectors and a micro USB connector.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 2 Mbyte QSPI flash
- 17 GPIO pins
- 3 ADC pins
- Supply voltage measurement
- I2C
- UART
- Micro USB connector
- LiPo charger
- Reset, boot and 2 user buttons
- 2 RGB LEDs (Neopixels)
- Piezo buzzer with mute switch
- 4 servo ports
- 2 Motor drivers, with 4 manual test bottons
- 7 Grove connectors, of which one can be used as a Maker/Qwiic/Stemma QT/zephyr_i2c connector
- 13 status indicators for digital pins


Default Zephyr Peripheral Mapping
=================================

+---------------+--------+-----------------------+-----------------+
| Description   | Pin    | Default pin mux       | Zephyr PWM name |
+===============+========+=======================+=================+
| RGB LEDs      | GPIO18 | PIO0                  |                 |
+---------------+--------+-----------------------+-----------------+
| Buzzer        | GPIO22 | PWM3A                 | 6               |
+---------------+--------+-----------------------+-----------------+
| Button 1      | GPIO20 | (Alias sw0)           |                 |
+---------------+--------+-----------------------+-----------------+
| Button 2      | GPIO21 | (Alias sw1)           |                 |
+---------------+--------+-----------------------+-----------------+
| Voltage sense | GPIO29 | ADC3, voltage divider |                 |
+---------------+--------+-----------------------+-----------------+

Servo header:

+-------+--------+-----------------+-----------------+
| Label | Pin    | Default pin mux | Zephyr PWM name |
+=======+========+=================+=================+
| GP12  | GPIO12 | PWM6A           | 12              |
+-------+--------+-----------------+-----------------+
| GP13  | GPIO13 | PWM6B           | 13              |
+-------+--------+-----------------+-----------------+
| GP14  | GPIO14 | PWM7A           | 14              |
+-------+--------+-----------------+-----------------+
| GP15  | GPIO15 | PWM7B           | 15              |
+-------+--------+-----------------+-----------------+

Motor drivers:

+-------+--------+-----------------+-----------------+
| Label | Pin    | Default pin mux | Zephyr PWM name |
+=======+========+=================+=================+
| M1A   | GPIO8  | PWM4A           | 8               |
+-------+--------+-----------------+-----------------+
| M1B   | GPIO9  | PWM4B           | 9               |
+-------+--------+-----------------+-----------------+
| M2A   | GPIO10 | PWM5A           | 10              |
+-------+--------+-----------------+-----------------+
| M2B   | GPIO11 | PWM5B           | 11              |
+-------+--------+-----------------+-----------------+

Grove connector 1:

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| GP0   | GPIO0  | UART0 TX        |
+-------+--------+-----------------+
| GP1   | GPIO1  | UART0 RX        |
+-------+--------+-----------------+

Grove connector 2:

Use an adapter cable to connect Qwiic/Stemma QT sensors (that use I2C) to this connector, which
is mapped to the ``zephyr_i2c`` devicetree node label.

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| GP2   | GPIO2  | I2C1 SDA        |
+-------+--------+-----------------+
| GP3   | GPIO3  | I2C1 SCL        |
+-------+--------+-----------------+

Grove connector 3:

+-------+--------+------------+
| Label | Pin    | Notes      |
+=======+========+============+
| GP4   | GPIO4  | Alias led0 |
+-------+--------+------------+
| GP5   | GPIO5  | Alias led1 |
+-------+--------+------------+

Grove connector 4:

+-------+--------+
| Label | Pin    |
+=======+========+
| GP16  | GPIO16 |
+-------+--------+
| GP17  | GPIO17 |
+-------+--------+

Grove connector 5:

+-------+--------+-----------------+-----------------+
| Label | Pin    | Default pin mux | Notes           |
+=======+========+=================+=================+
| GP6   | GPIO6  |                 |                 |
+-------+--------+-----------------+-----------------+
| GP26  | GPIO26 | ADC0            | Also in Grove 6 |
+-------+--------+-----------------+-----------------+

Grove connector 6:

+-------+--------+-----------------+-----------------+
| Label | Pin    | Default pin mux | Notes           |
+=======+========+=================+=================+
| GP26  | GPIO26 | ADC0            | Also in Grove 5 |
+-------+--------+-----------------+-----------------+
| GP27  | GPIO27 | ADC1            |                 |
+-------+--------+-----------------+-----------------+

Grove connector 7:

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| GP7   | GPIO7  |                 |
+-------+--------+-----------------+
| GP28  | GPIO28 | ADC2            |
+-------+--------+-----------------+

See also `schematic`_.


Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

By default programming is done via the USB connector. Press and hold the BOOT button, and then
press the RST button, and the device will appear as a USB mass storage unit.
Building your application will result in a :file:`build/zephyr/zephyr.uf2` file.
Drag and drop the file to the USB mass storage unit, and the board will be reprogrammed.

It is also possible to program and debug the board via the SWDIO and SWCLK pins in the DEBUG
connector. You must solder a 3-pin header to the board in order to use this feature.
A separate programming hardware tool is required, and for example the :command:`openocd` software
is used. Typically the ``OPENOCD`` and ``OPENOCD_DEFAULT_PATH`` values should be set when
building, and the ``--runner openocd`` argument should be used when flashing.
For more details on programming RP2040-based boards, see :ref:`rpi_pico_programming_and_debugging`.


Flashing
========

To run the :zephyr:code-sample:`blinky` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky/
   :board: maker_pi_rp2040
   :goals: build flash

Try also the :zephyr:code-sample:`led-strip`, :zephyr:code-sample:`hello_world`,
:zephyr:code-sample:`button`, :zephyr:code-sample:`input-dump` and
:zephyr:code-sample:`adc_dt` samples.

The use of the Maker/Qwiic/Stemma QT I2C connector (Grove 2) is demonstrated using the
:zephyr:code-sample:`light_sensor_polling` sample and a separate shield:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/light_polling
   :board: maker_pi_rp2040
   :shield: adafruit_veml7700
   :goals: build flash

Use the shell to control the GPIO pins:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: maker_pi_rp2040
   :gen-args: -DCONFIG_GPIO=y -DCONFIG_GPIO_SHELL=y
   :goals: build flash

To set one of the GPIO pins high, use these commands in the shell, and study the indicator LEDs:

.. code-block:: shell

    gpio conf gpio0 16 o
    gpio set gpio0 16 1

Servo motor control is done via PWM outputs. The :zephyr:code-sample:`servo-motor`
sample sets servo position timing (via an overlay file) for the output GP12:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/servo_motor/
   :board: maker_pi_rp2040
   :goals: build flash

It is also possible to control servos via the pwm shell:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: maker_pi_rp2040
   :gen-args: -DCONFIG_PWM=y -DCONFIG_PWM_SHELL=y
   :goals: build flash

Use shell commands to set the position of the servo. Most servo motors can handle pulse
times between 800 and 2000 microseconds:

.. code-block:: shell

    pwm usec pwm@40050000 12 20000 800
    pwm usec pwm@40050000 12 20000 2000

To use the buzzer, you must turn on the buzzer switch on the short side of the board.
Then build using the same command as above for the sensor_shell.
Use these shell commands to turn on and off the buzzer:

.. code-block:: shell

    pwm usec pwm@40050000 6 1000 500
    pwm usec pwm@40050000 6 1000 0

You can also control the motor outputs via the shell. To set the speed of motor 1 to
100%, 50%, 20% and 0% respectively, use these commands:

.. code-block:: shell

   pwm usec pwm@40050000 8 1000 1000
   pwm usec pwm@40050000 8 1000 500
   pwm usec pwm@40050000 8 1000 200
   pwm usec pwm@40050000 8 1000 0

To run the motor in the opposite direction at 80%:

.. code-block:: shell

   pwm usec pwm@40050000 9 1000 800

The sensor_shell sample is used also to measure the supply voltage. This is the result when
running on a Vin voltage slightly less than 6 Volt:

.. code-block:: shell

   sensor get vbatt
   channel type=33(voltage) index=0 shift=3 num_samples=1 value=32688380776ns (5.977999)


References
**********

.. target-notes::

.. _Cytron Maker Pi RP2040:
    https://www.cytron.io/p-maker-pi-rp2040-simplifying-robotics-with-raspberry-pi-rp2040

.. _schematic:
    https://drive.google.com/file/d/1BNqbxXScMXnL3-2YYfbR66nFCD1li71X/view
