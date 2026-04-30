.. zephyr:board:: stg_800

Overview
********

The `Barth Electronik STG-800 lococube mini-PLC`_ board is based on the STM32F091 microcontroller
from ST Microelectronics. It has analog and digital inputs, digital power outputs and a CAN bus
interface.

.. WARNING::
   If you program this board with Zephyr firmware, the original miCon-L firmware will be
   irreversibly overwritten.


Hardware
********

- Microcontroller STM32F091CCUx in UFQFPN48 package. Max frequency 48 MHz.
- ARM Cortex M0+ core
- 32 kByte SRAM
- 256 kByte flash
- 8 kByte EEPROM via I2C
- Supply voltage +7 to +32 Volt
- Temperature range -40 to +70 °C
- Vibration Resistant and Rugged Sealing
- Green user LED
- 3 analog inputs 0-30 Volt, 12 bit ADC
- 2 digital inputs
- 4 high-side digital outputs, capable of 1.5 A
- 1 low-side PWM digital output, capable of 2 A
- CAN bus interface
- Debug serial port, 3.3 Volt compatible
- Serial wire debug (SWD) connector


Default Zephyr Peripheral Mapping
=================================

+-------+--------+
| Label | Pin    |
+=======+========+
| LED   | PA8    |
+-------+--------+


EEPROM (8 kByte):

+--------+---------+-----------------+
| Signal | MCU pin | Default pin mux |
+========+=========+=================+
| SDA    | PB14    | I2C2            |
+--------+---------+-----------------+
| SCL    | PB13    | I2C2            |
+--------+---------+-----------------+


CAN controller:

+--------+---------+---------------------------------+
| Signal | MCU pin | Default pin mux                 |
+========+=========+=================================+
| CAN_TX | PB9     | CAN1                            |
+--------+---------+---------------------------------+
| CAN_RX | PB8     | CAN1                            |
+--------+---------+---------------------------------+
| CAN_S  | PB6     | GPIO for CAN transceiver enable |
+--------+---------+---------------------------------+


X1 Connector (Supply and CAN bus):

+---------------+--------+----------------------------------+
| Connector pin | Label  | Description                      |
+===============+========+==================================+
| 1             | +VDD   | +7 to +32 Volt, use fuse max 5 A |
+---------------+--------+----------------------------------+
| 2             | GND    |                                  |
+---------------+--------+----------------------------------+
| 3             | CANH   |                                  |
+---------------+--------+----------------------------------+
| 4             | CANL   |                                  |
+---------------+--------+----------------------------------+

There are no CAN termination resistors on this board.


X2 Connector (Inputs and outputs):

+---------------+--------+---------+------------------------+-----------------+
| Connector pin | Label  | MCU pin | Description            | Default pin mux |
+===============+========+=========+========================+=================+
| 1             | IN1    | PA0     | Analog in 0-30 Volt    | ADC_IN0         |
+---------------+--------+---------+------------------------+-----------------+
| 2             | IN2    | PA1     | Analog in 0-30 Volt    | ADC_IN1         |
+---------------+--------+---------+------------------------+-----------------+
| 3             | IN3    | PA2     | Analog in 0-30 Volt    | ADC_IN2         |
+---------------+--------+---------+------------------------+-----------------+
| 4             | IN4    | PA12    | Digital in max 30 Volt |                 |
+---------------+--------+---------+------------------------+-----------------+
| 5             | IN5    | PA15    | Digital in max 30 Volt |                 |
+---------------+--------+---------+------------------------+-----------------+
| 6             | OUT1   | PC13    | Digital out highside   |                 |
+---------------+--------+---------+------------------------+-----------------+
| 7             | OUT2   | PC14    | Digital out highside   |                 |
+---------------+--------+---------+------------------------+-----------------+
| 8             | OUT3   | PC15    | Digital out highside   |                 |
+---------------+--------+---------+------------------------+-----------------+
| 9             | OUT4   | PA4     | Digital out highside   |                 |
+---------------+--------+---------+------------------------+-----------------+
| 10            | OUT5   | PB7     | Digital out lowside    |                 |
+---------------+--------+---------+------------------------+-----------------+

There are pulldown resistors on all inputs. Do not switch the highside digital outputs (OUT1-OUT4)
faster than 100 Hz, due to heat disipation in the drivers.


X3 Connector (Programming):

+------------------------+---------+-----------+----------------+
| Connector pin          | MCU pin | Signal    | ST-Link/V2 pin |
+========================+=========+===========+================+
| 1, closest to ISP text |         | +3.3 Volt | 1              |
+------------------------+---------+-----------+----------------+
| 2                      |         | GND       | 4              |
+------------------------+---------+-----------+----------------+
| 3                      | PA13    | SWDIO     | 7              |
+------------------------+---------+-----------+----------------+
| 4                      | PA14    | SWCLK     | 9              |
+------------------------+---------+-----------+----------------+
| 5                      |         | RESET     | 15             |
+------------------------+---------+-----------+----------------+


X4 Connector (Debug serial port):

+-----+------------------------+---------+---------------+-----------------+
| Pin | Comment                | MCU pin | Signal        | Default pin mux |
+=====+========================+=========+===============+=================+
| 1   | Closest to TTL232 text |         | GND           |                 |
+-----+------------------------+---------+---------------+-----------------+
| 2   | Connect to host TX     | PA10    | RX (3.3 Volt) | USART1          |
+-----+------------------------+---------+---------------+-----------------+
| 3   | Connect to host RX     | PA9     | TX (3.3 Volt) | USART1          |
+-----+------------------------+---------+---------------+-----------------+


Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Connect a ST-Link/V2 programmer to the X3 connector of the board. See the table above for how the
pins of the ST-Link/V2 programmer should be connected to pins of the board connector.

In order to be able to program new firmware to the board, the "option bytes" of the microcontroller
must be erased to disable read/write protection. This can be done using the `STM32CubeProgrammer`_
software.

The board is configured to be flashed using west STM32CubeProgrammer runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, OpenOCD or JLink can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner jlink


Flashing
========

To run the :zephyr:code-sample:`blinky` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky/
   :board: stg_800
   :goals: build flash

Try also the :zephyr:code-sample:`hello_world`, :zephyr:code-sample:`adc_dt`,
:zephyr:code-sample:`eeprom`, :zephyr:code-sample:`can-counter` and
:zephyr:code-sample:`canopennode` samples.

Use the shell to read the input pins and control the output pins:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: stg_800
   :gen-args: -DCONFIG_GPIO=y -DCONFIG_GPIO_SHELL=y
   :goals: build flash

Read analog voltage on IN1 (which is around 12 Volt in this example):

.. code-block:: shell

   uart:~$ sensor get input1
   channel type=42(voltage) index=0 shift=4 num_samples=1 value=87626876000ns (11.984414)

Read digital signal on IN4 (pin A12):

.. code-block:: shell

   uart:~$ gpio conf gpioa 12 ih
   uart:~$ gpio get gpioa 12
   1

Set digital OUT1 (pin C13) to high:

.. code-block:: shell

   uart:~$ gpio conf gpioc 13 oh0
   uart:~$ gpio set gpioc 13 1


References
**********

.. target-notes::

.. _Barth Electronik STG-800 lococube mini-PLC:
    https://barth-elektronik.com/en/lococube-mini-PLC-STG-800/0850-0800

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
