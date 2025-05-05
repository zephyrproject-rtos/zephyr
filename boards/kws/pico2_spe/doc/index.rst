.. zephyr:board:: pico2_spe

Overview
********

The Pico2-SPE is a small, low-cost, versatile boards from
KWS Computersysteme Gmbh. They are equipped with an RP2350a SoC, an on-board LED,
a USB connector, an SWD interface. The Pico2-SPE additionally contains an
Microchip LAN8651 10Base-T1S module. The USB bootloader allows the
ability to flash without any adapter, in a drag-and-drop manner.
It is also possible to flash and debug the boards with their SWD interface,
using an external adapter.

Hardware
********

- Dual Cortex-M33 or Hazard3 processors at up to 150MHz
- 520KB of SRAM, and 4MB of on-board flash memory
- USB 1.1 with device and host support
- Low-power sleep and dormant modes
- Drag-and-drop programming using mass storage over USB
- 26 multi-function GPIO pins including 3 that can be used for ADC
- 1 SPI, 2 I2C, 2 UART, 3 12-bit 500ksps Analogue to Digital - Converter (ADC), 24 controllable PWM channels
- 2 Timer with 4 alarms, 1 AON Timer
- Temperature sensor
- Microchip LAN8651 10Base-T1S
- 3 Programmable IO (PIO) blocks, 12 state machines total for custom peripheral support

  - Flexible, user-programmable high-speed IO
  - Can emulate interfaces such as SD Card and VGA

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The default pin mapping is unchanged from the Pico-SPE.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

As with the Pico-SPE, the SWD interface can be used to program and debug the
device, e.g. using OpenOCD with the `Raspberry Pi Debug Probe <https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html>`_ .

References
**********

.. _Getting Started with Pico-SPE-Series:
   https://kws-computer.de/go/pico-spe-getting-started

.. _Pico2-SPE Documentation:
   https://kws-computer.de/go/pico2-spe-datasheet

.. target-notes::
