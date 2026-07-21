.. zephyr:board:: wio_tracker_l1

Overview
********

The Seeed Studio Wio Tracker L1 is a LoRa + GNSS tracker based on a Nordic Semiconductor nRF52840.
The board is sold as a Meshtastic node, with optional variants (notably the L1 Pro) that bundle the
same signal board with an enclosure and an integrated battery.

For more information, see the `Wio Tracker L1 product page`_ and the `Wio Tracker L1 wiki`_.

Hardware
********

The Wio Tracker L1 signal board carries:

- Nordic Semiconductor nRF52840 (ARM Cortex-M4F, 1 MB flash / 256 kB RAM, Bluetooth 5 and IEEE
  802.15.4 transceiver, native USB 2.0 full-speed).
- Semtech SX1262 LoRa modem (862-930 MHz) with TCXO (DIO3, 1.8 V) and an RF switch driven by DIO2
  plus a separate LNA-enable line on P1.08.
- Quectel L76K GNSS receiver (GPS / BeiDou) on UART0 at 9600 baud.
- 16 Mbit Puya P25Q16H QSPI flash.
- 1.3" SH1106 OLED on the primary I\ :sup:`2`\ C bus.
- External Grove I\ :sup:`2`\ C connector on TWIM1.
- One user LED, one user button, a buzzer (PWM), a 5-way trackball and a battery voltage divider
  gated by a control GPIO.
- USB Type-C connector wired to the nRF52840 native USB peripheral.

Connections and IOs
===================

+-------+-----------+--------------+----------------------------+
| Pin   | Direction | Function     | Usage                      |
+=======+===========+==============+============================+
| P1.01 | OUT       | GPIO         | User LED (active high)     |
+-------+-----------+--------------+----------------------------+
| P0.08 | IN        | GPIO         | User Button (active low)   |
+-------+-----------+--------------+----------------------------+
| P0.30 | OUT       | SPIM3 SCK    | SX1262 LoRa SCK            |
+-------+-----------+--------------+----------------------------+
| P0.03 | IN        | SPIM3 MISO   | SX1262 LoRa MISO           |
+-------+-----------+--------------+----------------------------+
| P0.28 | OUT       | SPIM3 MOSI   | SX1262 LoRa MOSI           |
+-------+-----------+--------------+----------------------------+
| P1.14 | OUT       | GPIO         | SX1262 NSS (chip select)   |
+-------+-----------+--------------+----------------------------+
| P0.07 | IN        | GPIO         | SX1262 DIO1 (IRQ)          |
+-------+-----------+--------------+----------------------------+
| P1.10 | IN        | GPIO         | SX1262 BUSY                |
+-------+-----------+--------------+----------------------------+
| P1.07 | OUT       | GPIO         | SX1262 RESET (active low)  |
+-------+-----------+--------------+----------------------------+
| P1.08 | OUT       | GPIO         | SX1262 RX-enable           |
+-------+-----------+--------------+----------------------------+
| P0.27 | OUT       | UART0 TX     | L76K GNSS RX               |
+-------+-----------+--------------+----------------------------+
| P0.26 | IN        | UART0 RX     | L76K GNSS TX               |
+-------+-----------+--------------+----------------------------+
| P0.05 | OUT       | TWIM0 SCL    | SH1106 OLED I\ :sup:`2`\ C |
+-------+-----------+--------------+----------------------------+
| P0.06 | I/O       | TWIM0 SDA    | SH1106 OLED I\ :sup:`2`\ C |
+-------+-----------+--------------+----------------------------+
| P1.11 | I/O       | TWIM1 SDA    | Grove I\ :sup:`2`\ C SDA   |
+-------+-----------+--------------+----------------------------+
| P1.12 | OUT       | TWIM1 SCL    | Grove I\ :sup:`2`\ C SCL   |
+-------+-----------+--------------+----------------------------+
| P1.04 | IN        | GPIO         | Trackball Up               |
+-------+-----------+--------------+----------------------------+
| P0.12 | IN        | GPIO         | Trackball Down             |
+-------+-----------+--------------+----------------------------+
| P0.11 | IN        | GPIO         | Trackball Left             |
+-------+-----------+--------------+----------------------------+
| P1.03 | IN        | GPIO         | Trackball Right            |
+-------+-----------+--------------+----------------------------+
| P1.05 | IN        | GPIO         | Trackball Press            |
+-------+-----------+--------------+----------------------------+
| P1.00 | OUT       | PWM0 OUT0    | Buzzer                     |
+-------+-----------+--------------+----------------------------+
| P0.04 | OUT       | GPIO         | Battery divider enable     |
+-------+-----------+--------------+----------------------------+
| P0.31 | AIN7      | ADC          | Battery voltage sense      |
+-------+-----------+--------------+----------------------------+
| P0.21 | OUT       | QSPI SCK     | Puya P25Q16H flash         |
+-------+-----------+--------------+----------------------------+
| P0.20 | I/O       | QSPI IO0     | Puya P25Q16H flash         |
+-------+-----------+--------------+----------------------------+
| P0.24 | I/O       | QSPI IO1     | Puya P25Q16H flash         |
+-------+-----------+--------------+----------------------------+
| P0.22 | I/O       | QSPI IO2     | Puya P25Q16H flash         |
+-------+-----------+--------------+----------------------------+
| P0.23 | I/O       | QSPI IO3     | Puya P25Q16H flash         |
+-------+-----------+--------------+----------------------------+
| P0.25 | OUT       | QSPI CSn     | Puya P25Q16H flash         |
+-------+-----------+--------------+----------------------------+

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The Wio Tracker L1 ships with a pre-installed Adafruit nRF52 bootloader that exposes a UF2 USB Mass
Storage interface when the user double-taps the RST button. The volume identifies itself as
``Seeed TRACKER L1`` with Board-ID ``TRACKER L1`` (see ``INFO_UF2.TXT`` on the mounted volume). The
default Zephyr workflow relies on this bootloader, so no external SWD probe is required for
day-to-day development.

.. warning::

   The bootloader lives in the top 48 kB of flash and is what makes the USB-only flashing flow
   possible. Erasing it (for example via an external SWD probe and ``nrfjprog --eraseall``) is a
   one-way trip without that probe: USB recovery will no longer work and the bootloader UF2 must be
   re-flashed through the SWD pads on the rear of the board.

Flashing
========

#. Build the application; the build system produces :file:`build/zephyr/zephyr.uf2` because
   :kconfig:option:`CONFIG_BUILD_OUTPUT_UF2` is enabled by default for this board.
#. Double-tap the RST button. The board re-enumerates as a USB MSC volume titled ``WTL1BOOT`` (or
   similar, depending on the bootloader revision).
#. Run ``west flash``. The :ref:`uf2 runner <runner_uf2>` copies ``zephyr.uf2`` onto the mounted
   volume; the bootloader writes the image and resets into the application.

Alternatively, drag and drop ``zephyr.uf2`` onto the volume manually.

``board.cmake`` already passes ``--board-id="TRACKER L1"`` so that the runner picks the Wio Tracker
L1 when multiple UF2-capable devices are mounted at once. Should the bootloader ever be rebuilt
with a different Board-ID, override the default on the command line:

.. code-block:: console

   west flash --runner uf2 --board-id "<Board-ID from INFO_UF2.TXT>"

Building and flashing the :zephyr:code-sample:`blinky` sample is the recommended smoke test:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: wio_tracker_l1
   :goals: build flash

References
**********

.. target-notes::

.. _Wio Tracker L1 product page:
   https://www.seeedstudio.com/Wio-Tracker-L1-Pro-p-6454.html

.. _Wio Tracker L1 wiki:
   https://wiki.seeedstudio.com/wio_tracker_l1_node/
