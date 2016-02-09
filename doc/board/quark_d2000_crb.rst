.. _quark_d2000_crb:

Quark D2000 CRB
###############

Overview
********
The Intel® Quark ™ microcontroller D2000 package is shipped as a 40-pin QFN
component.

Intel™ Quark® microcontroller D2000 contains the following items:
* On-board components:
  -Accelerometer/Magnetometer sensor
  - UART/JTAG to USB convert for USB debug port
* Expansion options:
  - “Arduino Uno” compatible SIL sockets ( 3.3V IO Only )
* Other connectors:
  - 1x USB 2.0 Device Port – micro Type B
  - On-board coin cell battery holder
  - 5V input a screw terminal/header (external power or Li-ion)
  - EEMBC power input header

Board Layout
************

General information for the board can be found at the `Intel Website`_,
which includes both `schematics`_ and BRD files.

Supported Features
******************

+-----------+------------+-----+-----+-----------------------+
| Interface | Controller | ARC | x86 | Driver/Component      |
+===========+============+=====+=====+=======================+
| MVIC      | on-chip    |  N  |  Y  | interrupt_controller  |
+-----------+------------+-----+-----+-----------------------+
| UART      | on-chip    |  N  |  Y  | serial port-polling;  |
|           |            |     |     | serial port-interrupt |
+-----------+------------+-----+-----+-----------------------+
| SPI       | on-chip    |  Y  |  Y  | spi                   |
+-----------+------------+-----+-----+-----------------------+
| ADC       | on-chip    |  Y  |  N  | adc                   |
+-----------+------------+-----+-----+-----------------------+
| I2C       | on-chip    |  Y  |  Y  | i2c                   |
+-----------+------------+-----+-----+-----------------------+
| GPIO      | on-chip    |  Y  |  Y  | gpio                  |
+-----------+------------+-----+-----+-----------------------+
| PWM       | on-chip    |  Y  |  Y  | pwm                   |
+-----------+------------+-----+-----+-----------------------+


Bibliography
************

.. _Intel Website:
   http://www.intel.com/content/www/us/en/embedded/products/quark/mcu/d2000/quark-d2000-crb-user-guide.html

.. _schematics:
   http://www.intel.com/content/www/us/en/embedded/products/quark/mcu/d2000/quark-d2000-crb-schematics.html

