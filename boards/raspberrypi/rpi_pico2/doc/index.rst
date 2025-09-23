.. zephyr:board:: rpi_pico2

Overview
********

The Raspberry Pi Pico 2 and Pico 2W are second-generation products in the Raspberry Pi
Pico family. From the `Raspberry Pi website <https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html>`_ is referred to as Pico 2.


There are many limitations of the board currently. Including but not limited to:
- The Zephyr build only supports configuring the RP2350A with the Cortex-M33 cores.
- As with the Pico 1, there's no support for running any code on the second core.

Hardware
********

- Dual Cortex-M33 or Hazard3 processors at up to 150MHz
- 520KB of SRAM, and 4MB of on-board flash memory
- USB 1.1 with device and host support
- Low-power sleep and dormant modes
- Drag-and-drop programming using mass storage over USB
- 26 multi-function GPIO pins including 3 that can be used for ADC
- 2 SPI, 2 I2C, 2 UART, 3 12-bit 500ksps Analogue to Digital - Converter (ADC), 24 controllable PWM channels
- 2 Timer with 4 alarms, 1 AON Timer
- Temperature sensor
- 3 Programmable IO (PIO) blocks, 12 state machines total for custom peripheral support
- Infineon CYW43439 2.4 GHz Wi-Fi chip (Pico 2W only)

  - Flexible, user-programmable high-speed IO
  - Can emulate interfaces such as SD Card and VGA

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The default pin mapping is unchanged from the Pico 1 (see :ref:`rpi_pico_pin_mapping`).

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The overall explanation regarding flashing and debugging is the same as for :zephyr:board:`rpi_pico`.
See :ref:`rpi_pico_programming_and_debugging` in :zephyr:board:`rpi_pico` documentation. N.b. OpenOCD support requires using Raspberry Pi's forked version of OpenOCD.

Below is an example of building and flashing the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
    :zephyr-app: samples/basic/blinky
    :board: rpi_pico2/rp2350a/m33
    :goals: build flash
    :flash-args: --openocd /usr/local/bin/openocd

The blinky sample is not yet supported on Pico 2W, so try the :zephyr:code-sample:`wifi-shell` application to connect to the network:

.. zephyr-app-commands::
    :zephyr-app: samples/net/wifi/shell
    :board: rpi_pico2/rp2350a/m33/w
    :goals: build flash
    :flash-args: --openocd /usr/local/bin/openocd

References
**********

.. target-notes::
