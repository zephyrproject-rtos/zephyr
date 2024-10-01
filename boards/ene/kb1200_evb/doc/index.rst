.. _ene_kb1200_evb:

ENE KB1200_EVB
##############

Overview
********

The KB1200_EVB kit is a development platform to evaluate the
ENE KB1200 series microcontrollers. This board needs to be mated with
part number KB1200.


Hardware
********

- ARM Cortex-M4F Processor
- 512KB Flash and 320KB RAM
- ADC & GPIO headers
- SER1, SER2 and SER3
- FAN PWM interface
- ENE Debug interface

Supported Features
==================

The following features are supported:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| ADC       | on-chip    | adc controller                      |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | reset and clock control             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c port/controller                 |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| PMU       | on-chip    | power management                    |
+-----------+------------+-------------------------------------+
| PSL       | on-chip    | power switch logic                  |
+-----------+------------+-------------------------------------+
| PWM       | on-chip    | pulse width modulator               |
+-----------+------------+-------------------------------------+
| TACH      | on-chip    | tachometer sensor                   |
+-----------+------------+-------------------------------------+
| SER       | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| WDT       | on-chip    | watchdog                            |
+-----------+------------+-------------------------------------+

Other hardware features are not currently supported by Zephyr (at the moment)

System Clock
============

The KB1200 MCU is configured to use the 96Mhz internal oscillator with the
on-chip DPLL to generate a resulting EC clock rate of 96MHz/48MHz/24MHz/12MHz.
See Processor clock control register (refer 5.1 General Configuration)

Programming and Debugging
*************************

Flashing
========

If the correct headers are installed, this board supports SWD Debug Interface.

To flash with SWD, install the drivers for your programmer, for example:
SEGGER J-link's drivers are at https://www.segger.com/downloads/jlink/

Debugging
=========

Use SWD with a J-Link

References
==========

.. target-notes::
