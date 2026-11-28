.. zephyr:board:: rak19026

The RAK19026 WisMesh Base Board carries a RAK4630 WisDuo module soldered to the
board rather than plugged into a Core slot, so it is a complete node on its
own: a Nordic nRF52840 with a Semtech SX1262 LoRa transceiver, a u-blox
ZOE-M8Q GNSS receiver, an ST LIS3DH accelerometer and an OLED display.

Hardware
********

- nRF52840 with 1 MB flash and 256 KB RAM
- SX1262 LoRa transceiver, IPEX antenna connector
- u-blox ZOE-M8Q GNSS receiver, IPEX antenna connector
- ST LIS3DH three-axis accelerometer
- Solomon SSD1306 monochrome OLED display, 128x64
- Green and blue user LEDs, and a red charger LED
- User button and reset button
- USB-C for power, charging and the console
- Li-Ion/LiPo charger and a solar panel input
- Two WisBlock sensor slots, C and D, and one WisBlock I/O slot

For more information about the board:

- `RAK19026 WisMesh Base Board Website`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The nRF52840 reaches the board through the RAK4630 module:

+---------------------+-----------+------------------------------------------+
| Function            | nRF52840  | Reaches                                  |
+=====================+===========+==========================================+
| I2C1 SDA/SCL        | P0.13/14  | LIS3DH, GNSS, OLED, slots C and D        |
+---------------------+-----------+------------------------------------------+
| I2C2 SDA/SCL        | P0.24/25  | I/O slot                                 |
+---------------------+-----------+------------------------------------------+
| UART0 TX/RX         | P0.20/19  | I/O slot                                 |
+---------------------+-----------+------------------------------------------+
| UART1 TX/RX         | P0.16/15  | GNSS, slots C and D, I/O slot            |
+---------------------+-----------+------------------------------------------+
| SPI CS/CLK/MISO/MOSI| P0.26/03/ | Slots C and D, I/O slot                  |
|                     | 29/30     |                                          |
+---------------------+-----------+------------------------------------------+
| Green LED           | P1.03     | On board                                 |
+---------------------+-----------+------------------------------------------+
| Blue LED            | P1.04     | On board                                 |
+---------------------+-----------+------------------------------------------+
| User button         | P0.09     | On board, and the I/O slot as IO5        |
+---------------------+-----------+------------------------------------------+
| Accelerometer INT1  | P0.17     | On board, and the I/O slot as IO1        |
+---------------------+-----------+------------------------------------------+
| Switched rail enable| P1.02     | 3V3_S, and the I/O slot as IO2           |
+---------------------+-----------+------------------------------------------+
| Battery sense       | P0.05     | On board, and the I/O slot as AIN0       |
+---------------------+-----------+------------------------------------------+

Three of those lines are shared with the slots, so a module that drives them
takes them from the board:

- IO5 is the user button
- IO1 is the accelerometer interrupt
- AIN0 is the battery divider, so a module reading it reads the battery

The GNSS receiver and everything in the sensor slots run from the switched
3V3_S rail. IO2 enables it, and a pull-up holds it on while nothing drives the
line, so the rail is up out of reset. ``wisblock_3v3_s`` is the power domain
over that line: a device that names it in ``power-domains`` keeps the rail on
while it is resumed, and with :kconfig:option:`CONFIG_PM_DEVICE` set the rail
goes down when no such device is.

The GNSS receiver answers on both of its interfaces, on UART1 at 9600 baud and
over I2C at address 0x42. The board uses UART1, which is also where a module
in a sensor slot would find it.

The board leaves three GNSS links unpopulated: the timepulse output (R39, to
IO3), the reset input (R45, to IO6) and the external interrupt (R44, to IO4).
Fitting R39 connects the timepulse to the pin sensor slot C uses for its
second GPIO, so the two cannot both be used.

Programming and Debugging
=========================

.. zephyr:board-supported-runners::

The board exposes SWD on the J7 header. Build and flash an application with
an external debug probe:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak19026
   :goals: build flash

The console is the USB CDC ACM port, so no serial adapter is needed. Open a
terminal on it at any speed and reset the board:

.. code-block:: console

   Hello World! rak19026/nrf52840

.. _RAK19026 WisMesh Base Board Website:
   https://docs.rakwireless.com/product-categories/meshtastic/wismesh-base/
