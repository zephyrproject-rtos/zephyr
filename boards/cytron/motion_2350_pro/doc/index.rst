.. zephyr:board:: motion_2350_pro

Overview
********

The `Cytron Motion 2350 Pro`_ board is based on the RP2350A microcontroller from Raspberry Pi Ltd.
The board has motor drivers, servo headers, Maker connectors and a USB type C connector.


Hardware
********

- Microcontroller Raspberry Pi RP2350A, with a max frequency of 150 MHz
- Dual ARM Cortex M33 cores, and dual RISC-V Hazard3 cores.
- 520 kByte SRAM
- 2 Mbyte QSPI flash
- 12 GPIO pins
- 4 ADC pins
- I2C
- UART
- USB type C connector
- USB host (USB type A connector)
- Reset, boot and 2 user buttons
- 2 RGB LEDs (Neopixels)
- Piezo buzzer with mute switch
- 8 servo ports
- 4 Motor drivers, with 8 manual test bottons
- 3 Maker connectors, of which one can be used as a Qwiic/Stemma QT/zephyr_i2c connector
- 16 status indicators for digital pins


Default Zephyr Peripheral Mapping
=================================

+---------------+--------+-----------------------------+-----------------+
| Description   | Pin    | Default pin mux             | Zephyr PWM name |
+===============+========+=============================+=================+
| Button 1      | GPIO20 | (Alias sw0)                 |                 |
+---------------+--------+-----------------------------+-----------------+
| Button 2      | GPIO21 | (Alias sw1)                 |                 |
+---------------+--------+-----------------------------+-----------------+
| Buzzer        | GPIO22 | PWM3A (also used for servo) | 6               |
+---------------+--------+-----------------------------+-----------------+
| RGB LEDs      | GPIO23 | PIO0                        |                 |
+---------------+--------+-----------------------------+-----------------+
| USB host D+   | GPIO24 |                             |                 |
+---------------+--------+-----------------------------+-----------------+
| USB host D-   | GPIO25 |                             |                 |
+---------------+--------+-----------------------------+-----------------+


GPIO header:

+-------+--------+-----------------+----------------------+
| Label | Pin    | Default pin mux | Also in connector    |
+=======+========+=================+======================+
| GP16  | GPIO16 | I2C0 SDA        | Maker port GP16+GP17 |
+-------+--------+-----------------+----------------------+
| GP17  | GPIO17 | I2C0 SCL        | Maker port GP16+GP17 |
+-------+--------+-----------------+----------------------+
| GP18  | GPIO18 | (Alias led0)    |                      |
+-------+--------+-----------------+----------------------+
| GP19  | GPIO19 | (Alias led1)    |                      |
+-------+--------+-----------------+----------------------+
| GP26  | GPIO26 | ADC0            | Maker port GP26+GP27 |
+-------+--------+-----------------+----------------------+
| GP27  | GPIO27 | ADC1            | Maker port GP26+GP27 |
+-------+--------+-----------------+----------------------+
| GP28  | GPIO28 | UART0 TX        | Maker port GP28+GP29 |
+-------+--------+-----------------+----------------------+
| GP29  | GPIO29 | UART0 RX        | Maker port GP28+GP29 |
+-------+--------+-----------------+----------------------+


Servo header (Note that PWM3A is also used for the buzzer):

+-------+--------+-----------------+-----------------+
| Label | Pin    | Default pin mux | Zephyr PWM name |
+=======+========+=================+=================+
| GP0   | GPIO0  | PWM0A           | 0               |
+-------+--------+-----------------+-----------------+
| GP1   | GPIO1  | PWM0B           | 1               |
+-------+--------+-----------------+-----------------+
| GP2   | GPIO2  | PWM1A           | 2               |
+-------+--------+-----------------+-----------------+
| GP3   | GPIO3  | PWM1B           | 3               |
+-------+--------+-----------------+-----------------+
| GP4   | GPIO4  | PWM2A           | 4               |
+-------+--------+-----------------+-----------------+
| GP5   | GPIO5  | PWM2B           | 5               |
+-------+--------+-----------------+-----------------+
| GP6   | GPIO6  | PWM3A           | 6               |
+-------+--------+-----------------+-----------------+
| GP7   | GPIO7  | PWM3B           | 7               |
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
| M3A   | GPIO12 | PWM6A           | 12              |
+-------+--------+-----------------+-----------------+
| M3B   | GPIO13 | PWM6B           | 13              |
+-------+--------+-----------------+-----------------+
| M4A   | GPIO14 | PWM7A           | 14              |
+-------+--------+-----------------+-----------------+
| M4B   | GPIO15 | PWM7B           | 15              |
+-------+--------+-----------------+-----------------+


Connector GP16+GP17:

Connect Qwiic/Stemma QT sensors (that use I2C) to this connector.

The pins are also available in the GPIO header.

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| GP16  | GPIO16 | I2C0 SDA        |
+-------+--------+-----------------+
| GP17  | GPIO17 | I2C0 SCL        |
+-------+--------+-----------------+


Connector GP26+GP27:

The pins are also available in the GPIO header.

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| GP26  | GPIO26 | ADC0            |
+-------+--------+-----------------+
| GP27  | GPIO27 | ADC1            |
+-------+--------+-----------------+


Connector GP28+GP29:

The pins are also available in the GPIO header.

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| GP28  | GPIO28 | UART0 TX        |
+-------+--------+-----------------+
| GP29  | GPIO29 | UART0 RX        |
+-------+--------+-----------------+

See also `pinout`_.


Supported Features
==================

Note that USB host (the big USB type A connector) is not yet supported.

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
is used. You might need to use Raspberry Pi's forked version of OpenOCD.
Typically the ``OPENOCD`` and ``OPENOCD_DEFAULT_PATH`` values should be set when
building, and the ``--runner openocd`` argument should be used when flashing.
For more details on programming RP2040-based and RP2350-based boards,
see :ref:`rpi_pico_programming_and_debugging`.


Flashing the M33 core
=====================

To run the :zephyr:code-sample:`blinky` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky/
   :board: motion_2350_pro/rp2350a/m33
   :goals: build flash

Try also the :zephyr:code-sample:`led-strip`, :zephyr:code-sample:`hello_world`,
:zephyr:code-sample:`button`, :zephyr:code-sample:`input-dump` and
:zephyr:code-sample:`adc_dt` samples.

The use of the Maker/Qwiic/Stemma QT I2C connector (GP16+GP17) is demonstrated using the
:zephyr:code-sample:`light_sensor_polling` sample and a separate shield:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/light_polling
   :board: motion_2350_pro/rp2350a/m33
   :shield: adafruit_veml7700
   :goals: build flash

Use the shell to control the GPIO pins:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: motion_2350_pro/rp2350a/m33
   :gen-args: -DCONFIG_GPIO=y -DCONFIG_GPIO_SHELL=y
   :goals: build flash

To set one of the GPIO pins high, use these commands in the shell, and study the indicator LEDs:

.. code-block:: shell

    gpio conf gpio0 18 o
    gpio set gpio0 18 1

Servo motor control is done via PWM outputs. The :zephyr:code-sample:`servo-motor`
sample sets servo position timing (via an overlay file) for the output GP0:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/servo_motor/
   :board: motion_2350_pro/rp2350a/m33
   :goals: build flash

It is also possible to control servos via the pwm shell:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: motion_2350_pro/rp2350a/m33
   :gen-args: -DCONFIG_PWM=y -DCONFIG_PWM_SHELL=y
   :goals: build flash

Use shell commands to set the position of the servo. Most servo motors can handle pulse
times between 800 and 2000 microseconds:

.. code-block:: shell

    pwm usec pwm@400a8000 0 20000 800
    pwm usec pwm@400a8000 0 20000 2000

To use the buzzer, you must turn on the buzzer switch on the short side of the board.
Then build using the same command as above for the sensor_shell.
Use these shell commands to turn on and off the buzzer:

.. code-block:: shell

    pwm usec pwm@400a8000 6 1000 500
    pwm usec pwm@400a8000 6 1000 0

You can also control the motor outputs via the shell. To set the speed of motor 1 to
100%, 50%, 20% and 0% respectively, use these commands:

.. code-block:: shell

   pwm usec pwm@400a8000 8 1000 1000
   pwm usec pwm@400a8000 8 1000 500
   pwm usec pwm@400a8000 8 1000 200
   pwm usec pwm@400a8000 8 1000 0

To run the motor in the opposite direction at 80% speed:

.. code-block:: shell

   pwm usec pwm@400a8000 9 1000 800


Flashing the Hazard3 core
=========================

The RP2350A microcontroller has two ARM M33 cores and two RISC-V Hazard3 cores.
To flash one of the Hazard3 cores, use the board argument ``motion_2350_pro/rp2350a/hazard3``.
The samples :zephyr:code-sample:`blinky` and :zephyr:code-sample:`input-dump` have been
verified for this core. Use the USB mass storage programming method described above.


References
**********

.. target-notes::

.. _Cytron Motion 2350 Pro:
    https://www.cytron.io/p-motion-2350-pro

.. _pinout:
   https://static.cytron.io/image/catalog/products/MOTION-2350-PRO/pinout-diagram-motion-2350-pro3.png
