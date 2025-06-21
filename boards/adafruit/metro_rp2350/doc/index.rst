.. zephyr:board:: adafruit_metro_rp2350

Overview
********

Choo! Choo! This is the RP2350 Metro Line, making all station stops at "Dual Cortex M33 mountain",
"528K RAM round-about" and "16 Megabytes of Flash town". This train is piled high with hardware that
complements the Raspberry Pi RP2350 chip to make it an excellent development board for projects that
want Arduino-shape-compatibility or just need the extra space and debugging ports.

The Adafruit Metro RP2350 is the second Metro board to use the Rasperry Pi Pico family.

There are many limitations of the board currently. Including but not limited to:
- The Zephyr build only supports configuring the RP2350B with the Cortex-M33 cores.
- As with other RP2040/RP2350 devices, there's no support for running any code on the second core.

Hardware
********

- RP2350 main chip, 150MHz clock, 3.3V logic
- 16 MB of QSPI flash for program storage
- 37 Available GPIO: 23 on the socket/SPI headers, 12 on HSTX port, and another 2 for USB host. 6 of which are also analog inputs
- Micro SD card socket wired up for SPI interfacing, also has extra pins connected for advanced-user SDIO interfacing (note that there's no code for SDIO in Arduino/Python, so this is a super-cutting-edge setup)
- 5V Buck Converter featuring TPS563201 6~17V DC input and up to 2A output
- Onboard RGB NeoPixel
- Onboard #23 LED
- Stemma QT port for I2C peripherals and sensors
- 22-pin 3-lane differential HSTX FPC port with 'Pi 5' compatible pinout, makes for quick DVI video output. Or, this also provides 12 extra GPIO that can be used for more pins.
- Reset and Boot buttons on PCB edge
- Pico Probe debug port - 3 pin JST SH compatible
- USB Type C power and data
- 5.5mm / 2.1mm DC jack for 6-17VDC power
- On/off switch for DC jack
- RX / TX switch for swapping D0 and D1 locations
- USB Host breakout pads - with controllable 5V power and D+/D- for bitbang USB Host.
- GPIO pin numbers match classic Arduino pins, other than GPIO 12 and 13 as those are needed for HSTX connectivity

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The default pin mapping is unchanged from the Pico 1 (see :ref:`rpi_pico_pin_mapping`).

Programming and Debugging
*************************

As with other RP3250 devices, the SWD interface can be used to program and debug the
device, e.g. using OpenOCD with the `Raspberry Pi Debug Probe <https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html>`_ .

References
**********

.. target-notes::
