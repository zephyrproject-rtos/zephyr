.. _arduino_nano_33_ble:

Arduino Nano 33 BLE (Sense)
#################################

Overview
********

The Arduino Nano 33 BLE is designed around Nordic Semiconductor's
nRF52840 ARM Cortex-M4F CPU. Arduino sells 2 variants of the board, the
plain `BLE`_ type and the `BLE Sense`_ type. The "Sense" variant is distinguished by
the inclusion of more sensors, but otherwise both variants are the same.

.. image:: img/arduino_nano_33_ble_sense.jpg
     :align: center
     :alt: Arduino Nano 33 BLE (Sense variant)

The Sense variant of the board

Hardware
********

Supported Features
==================

The package is configured to support the following hardware:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| ADC       | on-chip    | adc                  |
+-----------+------------+----------------------+
| CLOCK     | on-chip    | clock_control        |
+-----------+------------+----------------------+
| FLASH     | on-chip    | flash                |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| I2C0      | on-chip    | i2c                  |
+-----------+------------+----------------------+
| I2C1      | on-chip    | i2c                  |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| RADIO     | on-chip    | Bluetooth,           |
|           |            | ieee802154           |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| SPI       | on-chip    | spi                  |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| USB       | on-chip    | usb                  |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Other hardware features have not been enabled yet for this board.

Notably, this includes the PDM (microphone) interface.

Connections and IOs
===================

The `schematic`_ will tell you everything
you need to know about the pins.

The I2C pull-ups are enabled by setting pin P1.00 high. This is automatically
done at system init. The pin is specified in the ``zephyr,user`` Devicetree node
as ``pull-up-gpios``.

Programming and Debugging
*************************

This board requires the Arduino variant of bossac. You will not
be able to flash with the bossac included with the zephyr-sdk, or
using shumatech's mainline build.

You can get this variant of bossac with one of two ways:

#. Building the binary from the `Arduino source tree <https://github.com/arduino/BOSSA/tree/nrf>`_
#. Downloading the Arduino IDE

   #. Install the board support package within the IDE
   #. Change your IDE preferences to provide verbose logging
   #. Build and flash a sample application, and read the logs to figure out where Arduino stored bossac.
   #. In most Linux based systems the path is ``$HOME/.arduino15/packages/arduino/tools/bossac/1.9.1-arduino2/bossac``.

Once you have a path to bossac, you can pass it as an argument to west:

.. code-block:: bash

   west flash --bossac="<path to the arduino version of bossac>"

For example

.. code-block:: bash

    west flash --bossac=$HOME/.arduino15/packages/arduino/tools/bossac/1.9.1-arduino2/bossac

Flashing
========

Attach the board to your computer using the USB cable, and then

   .. zephyr-app-commands::
      :zephyr-app: samples/basic/blinky
      :board: arduino_nano_33_ble
      :goals: build
      :compact:

Double-tap the RESET button on your board. Your board should disconnect, reconnect,
and there should be a pulsing orange LED near the USB port.

Then, you can flash the image using the above script.

You should see the the red LED blink.

References
**********

.. target-notes::

.. _BLE:
    https://store.arduino.cc/products/arduino-nano-33-ble

.. _BLE SENSE:
    https://store.arduino.cc/products/arduino-nano-33-ble-sense

.. _pinouts:
    https://learn.adafruit.com/introducing-the-adafruit-nrf52840-feather/pinouts

.. _schematic:
    https://content.arduino.cc/assets/NANO33BLE_V2.0_sch.pdf
