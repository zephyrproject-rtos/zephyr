.. _ti_tmp116_sample:

TI_TMP116 Sample
################

Description
***********

This sample application periodically takes temperature readings using the ti_tmp116
sensor driver. The result is written to the console.

Requirements
************

This sample needs a TI TMP116 sensor connected to the target board's I2C
connector.


Wiring
******

This sample is tested with Nucleo STM32F401RE board.

The sensor operates at 3.3V and uses I2C to communicate with the board.

External Wires:

* Breakout **GND** pin <--> Nucleo **GND** pin
* Breakout **VCC** pin <--> Nucleo **3V3** pin
* Breakout **SDA** pin <--> Nucleo **CN5-D14** pin
* Breakout **SCL** pin <--> Nucleo **CN5-D15** pin

Building and Running
********************

In order to build the sample, connect the board to the computer with a USB cable and enter the
following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/tmp116
   :board: nucleo_f401re
   :goals: build flash
   :compact:

Sample Output
*************
The output can be seen via a terminal emulator (e.g. minicom). Connect the board with a USB cable
to the computer and open /dev/ttyACM0 with the below serial settings:

* Baudrate: 115200
* Parity: None
* Data: 8
* Stop bits: 1

The output should look like this:

.. code-block:: console

    Device TMP116 - 0x200010a8 is ready
    temp is 26.7031250 oC
    temp is 26.7109375 oC
    ...
