.. zephyr:board:: xiao_nrf52840_plus

Overview
********

The `Seeed Studio XIAO nRF52840 Plus`_ is a XIAO form factor development board built around the
Nordic Semiconductor nRF52840 SoC. It is electrically equivalent to the :zephyr:board:`xiao_ble`
and adds nine pins, exposed as pads on the bottom side of the board, next to the 14 castellated
pins of the XIAO form factor. The XIAO nRF52840 Sense Plus additionally embeds a 6-axis IMU and a
PDM microphone.

The two boards are supported by the following board targets:

- ``xiao_nrf52840_plus`` for the XIAO nRF52840 Plus
- ``xiao_nrf52840_plus/nrf52840/sense`` for the XIAO nRF52840 Sense Plus

Hardware
********

- Nordic nRF52840 Cortex-M4F processor at 64 MHz, 1 MB flash, 256 KB RAM
- 2 MB QSPI flash
- RGB LED
- USB Type-C connector, nRF52840 acting as USB device
- Battery charger BQ25101, with battery voltage measurement
- Reset button
- Bluetooth antenna, NFC antenna pins
- LSM6DS3TR-C 6-axis IMU (Sense Plus only)
- PDM microphone (Sense Plus only)

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `XIAO nRF52840 wiki`_ has detailed information about the board, including pinouts and
schematics.

The 14 castellated pins follow the standard XIAO pinout and are exposed through the ``xiao_d``
GPIO nexus node (see :dtcompatible:`seeed,xiao-gpio`) and the ``xiao_i2c``, ``xiao_spi``,
``xiao_serial`` and ``xiao_adc`` node labels.

The nine additional pads are exposed through the ``xiao_plus_d`` GPIO nexus node (see
:dtcompatible:`seeed,xiao-plus-gpio`), which uses the pad number as index, so
``<&xiao_plus_d 17 0>`` refers to D17. The secondary UART and SPI buses routed to these pads are
available as ``xiao_plus_serial`` and ``xiao_plus_spi``. Both are disabled by default.

.. list-table::
   :header-rows: 1

   * - Pin
     - GPIO
     - Function
     - Notes
   * - D11
     - P0.15
     - I2S SD
     -
   * - D12
     - P0.19
     - I2S SCK
     -
   * - D13
     - P1.01
     - I2S WS
     -
   * - D14
     - P0.09
     - ``xiao_plus_serial`` RX
     - NFC1, see below
   * - D15
     - P0.10
     - ``xiao_plus_serial`` TX
     - NFC2, see below
   * - D16
     - P0.31
     - AIN7
     - Battery voltage divider, see below
   * - D17
     - P1.03
     - ``xiao_plus_spi`` SCK
     -
   * - D18
     - P1.05
     - ``xiao_plus_spi`` MISO
     -
   * - D19
     - P1.07
     - ``xiao_plus_spi`` MOSI
     -

D14 and D15 are the NFC antenna pins of the nRF52840. To use them as GPIOs or as UART pins, the
``nfct-pins-as-gpios`` property must be set in the ``uicr`` node, which disables NFC:

.. code-block:: devicetree

   &uicr {
           nfct-pins-as-gpios;
   };

D16 is connected to the battery voltage divider. The battery voltage can be read using the
``vbatt`` node (:dtcompatible:`voltage-divider`), for example with the
``sensor get battery-divider`` command of the sensor shell. The driver connects the divider to
ground by driving P0.14 low while the ``vbatt`` device is active.

.. note::

   The XIAO ESP32S3 Plus places D17 and D19 at swapped positions compared to the XIAO nRF52840
   Plus. The ``xiao_plus_d`` indices always follow the pad names of the module pinout.

LEDs
----

* LED0 (red) = P0.26
* LED1 (green) = P0.30
* LED2 (blue) = P0.06

The red charge LED is driven by the battery charger. Its status can be read on P0.17.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The board ships with the `Adafruit nRF52 Bootloader`_, which supports flashing using `UF2`_ and is
the default runner. Flashing using the bootloader does not support debugging the device, for
debugging use an :ref:`External Debug Probe <debug-probes>` connected to the SWD pads on the
bottom side of the board.

Flashing
========

To enter the bootloader, connect the USB port of the board to your host and double tap the reset
button. A mass storage device should appear on the host. Build and flash the application in the
usual way, for example for the :zephyr:code-sample:`hello_world` application:

.. tabs::

   .. group-tab:: XIAO nRF52840 Plus

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: xiao_nrf52840_plus
         :goals: build flash

   .. group-tab:: XIAO nRF52840 Sense Plus

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: xiao_nrf52840_plus/nrf52840/sense
         :goals: build flash

The application console is available on the USB CDC ACM serial port.

Alternatively, copy the :file:`zephyr/zephyr.uf2` file from the build directory to the mass storage
device. The board resets and runs the new application once the copy is complete.

References
**********

.. target-notes::

.. _Seeed Studio XIAO nRF52840 Plus: https://www.seeedstudio.com/Seeed-Studio-XIAO-nRF52840-Plus-p-6359.html
.. _XIAO nRF52840 wiki: https://wiki.seeedstudio.com/XIAO_BLE/
.. _Adafruit nRF52 Bootloader: https://github.com/adafruit/Adafruit_nRF52_Bootloader
.. _UF2: https://github.com/microsoft/uf2
