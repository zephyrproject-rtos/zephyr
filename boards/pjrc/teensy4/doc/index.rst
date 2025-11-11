.. _teensy40:

PJRC TEENSY 4
#############

Overview
********

The Teensy is a complete USB-based microcontroller development system, in a
very small footprint, capable of implementing many types of projects. All
programming is done via the USB port.

.. tabs::

   .. group-tab:: Teensy 4.0

      .. figure:: img/teensy40.jpg
           :align: center
           :alt: TEENSY40

           (Credit: https://www.pjrc.com)

   .. group-tab:: Teensy 4.1

      .. figure:: img/teensy41.jpg
           :align: center
           :alt: TEENSY41

           (Credit: https://www.pjrc.com)

   .. group-tab:: Sparkfun Teensy Micromod

      .. figure:: img/teensymm.webp
           :align: center
           :alt: TEENSYMM

           (Credit: https://www.sparkfun.com)

Hardware
********

.. tabs::

   .. group-tab:: Teensy 4.0

      - MIMXRT1062DVL6A MCU (600 MHz, 1024 KB on-chip memory)
      - 16 Mbit QSPI Flash
      - User LED
      - USB 2.0 host connector

      See the `Teensy 4.0 Website`_ for a complete hardware description.

   .. group-tab:: Teensy 4.1

      - MIMXRT1062DVJ6A MCU (600 MHz, 1024 KB on-chip memory)
      - 64 Mbit QSPI Flash
      - User LED
      - USB 2.0 host connector
      - USB 2.0 OTG connector
      - 10/100 Mbit/s Ethernet transceiver
      - TF socket for SD card

      To connect an Ethernet cable, additional `Teensy 4.1 Ethernet Kit`_ is required.

      See the `Teensy 4.1 Website`_ for a complete hardware description.

   .. group-tab:: Sparkfun Teensy Micromod

      - MIMXRT1062DVJ6A MCU (600 MHz, 1024 KB on-chip memory)
      - 128 Mbit QSPI Flash
      - User LED
      - USB 2.0 host connector
      - USB 2.0 OTG connector
      - TF socket for SD card

      See the `Teensy Micromod Website`_ for a complete hardware description.

For more information, check the `i.MX RT1060 Datasheet`_.

Supported Features
==================

The Teensy 4.0 board configuration supports the following hardware
features:

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
| I2C       | on-chip    | i2c                  |
+-----------+------------+----------------------+
| I2S       | on-chip    | i2s                  |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| SPI       | on-chip    | spi                  |
+-----------+------------+----------------------+
| CAN       | on-chip    | can                  |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| USB       | on-chip    | usb                  |
+-----------+------------+----------------------+
| TRNG      | on-chip    | entropy              |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

The Teensy 4.1 board configuration supports additional hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| SDHC      | on-chip    | disk access                         |
+-----------+------------+-------------------------------------+
| ENET      | on-chip    | ethernet                            |
+-----------+------------+-------------------------------------+

Other hardware features have not been enabled yet for this board.

Connections and IOs
===================

Pin mappings from Teensy to MIMXRT1062 SoC.

+-----+------------+-------------------------------------+
| Pin | Pad ID     | Usage                               |
+=====+============+=====================================+
|  0  | AD_B0_03   | GPIO1_3 / UART6_RX / CAN2_RX        |
+-----+------------+-------------------------------------+
|  1  | AD_B0_02   | GPIO1_2 / UART6_TX / CAN2_TX        |
+-----+------------+-------------------------------------+
|  2  | EMC_04     | GPIO4_4                             |
+-----+------------+-------------------------------------+
|  3  | EMC_05     | GPIO4_5                             |
+-----+------------+-------------------------------------+
|  4  | EMC_06     | GPIO4_6                             |
+-----+------------+-------------------------------------+
|  5  | EMC_08     | GPIO4_8                             |
+-----+------------+-------------------------------------+
|  6  | B0_10      | GPIO2_10                            |
+-----+------------+-------------------------------------+
|  7  | B1_01      | GPIO2_17 / UART4_RX                 |
+-----+------------+-------------------------------------+
|  8  | B1_00      | GPIO2_16 / UART4_TX                 |
+-----+------------+-------------------------------------+
|  9  | B0_11      | GPIO2_11                            |
+-----+------------+-------------------------------------+
| 10  | B0_00      | GPIO2_0                             |
+-----+------------+-------------------------------------+
| 11  | B0_02      | GPIO2_2                             |
+-----+------------+-------------------------------------+
| 12  | B0_01      | GPIO2_1                             |
+-----+------------+-------------------------------------+
| 13  | B0_03      | GPIO2_3 / LED                       |
+-----+------------+-------------------------------------+
| 14  | AD_B1_02   | GPIO1_18 / UART2_TX                 |
+-----+------------+-------------------------------------+
| 15  | AD_B1_03   | GPIO1_19 / UART2_RX                 |
+-----+------------+-------------------------------------+
| 16  | AD_B1_07   | GPIO1_23 / UART3_RX / I2C3_SCL      |
+-----+------------+-------------------------------------+
| 17  | AD_B1_06   | GPIO1_22 / UART3_TX / I2C3_SDA      |
+-----+------------+-------------------------------------+
| 18  | AD_B1_01   | GPIO1_17 / I2C1_SDA                 |
+-----+------------+-------------------------------------+
| 19  | AD_B1_00   | GPIO1_16 / I2C1_SCL                 |
+-----+------------+-------------------------------------+
| 20  | AD_B1_10   | GPIO1_26 / UART8_TX                 |
+-----+------------+-------------------------------------+
| 21  | AD_B1_11   | GPIO1_27 / UART8_RX                 |
+-----+------------+-------------------------------------+
| 22  | AD_B1_08   | GPIO1_24 / CAN1_TX                  |
+-----+------------+-------------------------------------+
| 23  | AD_B1_09   | GPIO1_25 / CAN1_RX                  |
+-----+------------+-------------------------------------+
| 24  | AD_B0_12   | GPIO1_12 / UART1_TX / I2C4_SCL      |
+-----+------------+-------------------------------------+
| 25  | AD_B0_13   | GPIO1_13 / UART1_RX / I2C4_SDA      |
+-----+------------+-------------------------------------+
| 26  | AD_B1_14   | GPIO1_30 / SPI3_MOSI                |
+-----+------------+-------------------------------------+
| 27  | AD_B1_15   | GPIO1_31 / SPI3_SCK                 |
+-----+------------+-------------------------------------+
| 28  | EMC_32     | GPIO3_18 / UART7_RX                 |
+-----+------------+-------------------------------------+
| 29  | EMC_31     | GPIO4_31 / UART7_TX                 |
+-----+------------+-------------------------------------+
| 30  | EMC_37     | GPIO3_23 / CAN3_RX                  |
+-----+------------+-------------------------------------+
| 31  | EMC_36     | GPIO3_22 / CAN3_TX                  |
+-----+------------+-------------------------------------+
| 32  | B0_12      | GPIO2_12                            |
+-----+------------+-------------------------------------+
| 33  | EMC_07     | GPIO4_7                             |
+-----+------------+-------------------------------------+

Only Teensy 4.0 and Teensy Micromod:

+-----+------------+-------------------------------------+
| 34  | SD_B0_03   | GPIO3_15                            |
+-----+------------+-------------------------------------+
| 35  | SD_B0_02   | GPIO3_14                            |
+-----+------------+-------------------------------------+
| 36  | SD_B0_01   | GPIO3_13                            |
+-----+------------+-------------------------------------+
| 37  | SD_B0_00   | GPIO3_12                            |
+-----+------------+-------------------------------------+
| 38  | SD_B0_05   | GPIO3_17                            |
+-----+------------+-------------------------------------+
| 39  | SD_B0_04   | GPIO3_16                            |
+-----+------------+-------------------------------------+

Only Teensy Micromod

+-----+------------+-------------------------------------+
| 40  | B0_04      | GPIO2_4 / I2C2 SCL                  |
+-----+------------+-------------------------------------+
| 41  | B0_05      | GPIO2_5 / I2C2 SDA                  |
+-----+------------+-------------------------------------+
| 42  | B0_06      | GPIO2_6                             |
+-----+------------+-------------------------------------+
| 43  | B0_07      | GPIO2_7                             |
+-----+------------+-------------------------------------+
| 44  | B0_08      | GPIO2_8 / UART3 TX                  |
+-----+------------+-------------------------------------+
| 45  | B0_09      | GPIO2_9 / UART3 RX                  |
+-----+------------+-------------------------------------+

Only Teensy 4.1:

+-----+------------+-------------------------------------+
| 34  | B1_13      | GPIO2_29 / UART5_RX                 |
+-----+------------+-------------------------------------+
| 35  | B1_12      | GPIO2_28 / UART5_TX                 |
+-----+------------+-------------------------------------+
| 36  | B1_02      | GPIO2_18                            |
+-----+------------+-------------------------------------+
| 37  | B1_03      | GPIO2_19                            |
+-----+------------+-------------------------------------+
| 38  | AD_B1_12   | GPIO1_28 / SPI3_CS                  |
+-----+------------+-------------------------------------+
| 39  | AD_B1_13   | GPIO1_29 / SPI3_MISO                |
+-----+------------+-------------------------------------+
| 40  | AD_B1_04   | GPIO1_20                            |
+-----+------------+-------------------------------------+
| 41  | AD_B1_05   | GPIO1_21 / UART3_RX                 |
+-----+------------+-------------------------------------+

Pin mappings from Teensy Micromod pins to MIMXRT1062 SoC.

Teensy Micromod only:

+-----+-----+------+------------+-----------------------------------+
|MMOD | MMC | Pin  | Pad ID     | Usage                             |
+=====+=====+======+============+===================================+
|   8 |  16 |  27  | AD_B1_15   | <gpio1 31>  / SPI3_SCK            |
+-----+-----+------+------------+-----------------------------------+
|  10 |   2 |   4  | EMC_06     | <gpio4 6>                         |
+-----+-----+------+------------+-----------------------------------+
|  12 |     |  18  | AD_B1_01   | <gpio1 17>  / I2C1_SDA            |
+-----+-----+------+------------+-----------------------------------+
|  14 |     |  19  | AD_B1_00   | <gpio1 16>  / I2C1_SCL            |
+-----+-----+------+------------+-----------------------------------+
|  16 |  4  |  29  | EMC_31     | <gpio4 31>  / UART7_TX            |
+-----+-----+------+------------+-----------------------------------+
|  17 |     |   1  | AD_B0_02   | <gpio1 2>  / UART6_TX / CAN2_TX   |
+-----+-----+------+------------+-----------------------------------+
|  18 |  3  |   5  | EMC_08     | <gpio4 8>                         |
+-----+-----+------+------------+-----------------------------------+
|  19 |     |   0  | AD_B0_03   | <gpio1 3>  / UART6_RX / CAN2_RX   |
+-----+-----+------+------------+-----------------------------------+
|  20 |     |  16  | AD_B1_07   | <gpio1 23>  / UART3_RX / I2C3_SCL |
+-----+-----+------+------------+-----------------------------------+
|  22 |     |  17  | AD_B1_06   | <gpio1 22>  / UART3_TX / I2C3_SDA |
+-----+-----+------+------------+-----------------------------------+
|  32 |     |   3  | EMC_05     | <gpio4 5>                         |
+-----+-----+------+------------+-----------------------------------+
|  34 |  0  |  14  | AD_B1_02   | <gpio1 18>  / UART2_TX            |
+-----+-----+------+------------+-----------------------------------+
|  38 |  1  |  15  | AD_B1_03   | <gpio1 19>  / UART2_RX            |
+-----+-----+------+------------+-----------------------------------+
|  4  |     |  28  | EMC_32     | <gpio3 18>  / UART7_RX            |
+-----+-----+------+------------+-----------------------------------+
|  40 |  5  |  40  | B0_04      | <gpio2 04>  / I2C2 SCL            |
+-----+-----+------+------------+-----------------------------------+
|  41 |     |  30  | EMC_37     | <gpio3 23>  / CAN3_RX             |
+-----+-----+------+------------+-----------------------------------+
|  42 |  6  |  41  | B0_05      | <gpio2 05>  / I2C2 SDA            |
+-----+-----+------+------------+-----------------------------------+
|  43 |     |  31  | EMC_36     | <gpio3 22>  / CAN3_TX             |
+-----+-----+------+------------+-----------------------------------+
|  44 |  7  |  42  | B0_06      | <gpio2 06>                        |
+-----+-----+------+------------+-----------------------------------+
|  46 |  8  |  43  | B0_07      | <gpio2 07>                        |
+-----+-----+------+------------+-----------------------------------+
|  47 |     |   2  | EMC_04     | <gpio4 4>                         |
+-----+-----+------+------------+-----------------------------------+
|  48 |  9  |  44  | B0_08      | <gpio2 08>  / UART3 TX            |
+-----+-----+------+------------+-----------------------------------+
|  49 |     |  22  | AD_B1_08   | <gpio1 24>  / CAN1_TX             |
+-----+-----+------+------------+-----------------------------------+
|  50 |     |  21  | AD_B1_11   | <gpio1 27>  / UART8_RX            |
+-----+-----+------+------------+-----------------------------------+
|  51 |     |  25  | AD_B0_13   | <gpio1 13>  / UART1_RX / I2C4_SDA |
+-----+-----+------+------------+-----------------------------------+
|  52 |     |  20  | AD_B1_10   | <gpio1 26>  / UART8_TX            |
+-----+-----+------+------------+-----------------------------------+
|  53 |     |  24  | AD_B0_12   | <gpio1 12>  / UART1_TX / I2C4_SCL |
+-----+-----+------+------------+-----------------------------------+
|  54 |     |   8  | B1_00      | <gpio2 16>  / UART4_TX            |
+-----+-----+------+------------+-----------------------------------+
|  55 | 17  |  10  | B0_00      | <gpio2 0>                         |
+-----+-----+------+------------+-----------------------------------+
|  56 |     |   7  | B1_01      | <gpio2 17>  / UART4_RX            |
+-----+-----+------+------------+-----------------------------------+
|  57 |     |  13  | B0_03      | <gpio2 3>  / LED                  |
+-----+-----+------+------------+-----------------------------------+
|  58 |     |  23  | AD_B1_09   | <gpio1 25>  / CAN1_RX             |
+-----+-----+------+------------+-----------------------------------+
|  59 |     |  11  | B0_02      | <gpio2 2>                         |
+-----+-----+------+------------+-----------------------------------+
|  60 |     |  36  | SD_B0_01   | <gpio3 13>                        |
+-----+-----+------+------------+-----------------------------------+
|  61 |     |  12  | B0_01      | <gpio2 1>                         |
+-----+-----+------+------------+-----------------------------------+
|  62 |     |  37  | SD_B0_00   | <gpio3 12>                        |
+-----+-----+------+------------+-----------------------------------+
|  63 | 15  |  33  | EMC_07     | <gpio4 7>                         |
+-----+-----+------+------------+-----------------------------------+
|  64 |     |  35  | SD_B0_02   | <gpio3 14>                        |
+-----+-----+------+------------+-----------------------------------+
|  65 | 14  |  32  | B0_12      | <gpio2 12>                        |
+-----+-----+------+------------+-----------------------------------+
|  66 |     |  34  | SD_B0_03   | <gpio3 15>                        |
+-----+-----+------+------------+-----------------------------------+
|  67 | 13  |  26  | AD_B1_14   | <gpio1 30>  / SPI3_MOSI           |
+-----+-----+------+------------+-----------------------------------+
|  68 |     |  38  | SD_B0_05   | <gpio3 16>                        |
+-----+-----+------+------------+-----------------------------------+
|  69 | 12  |   9  | B0_11      | <gpio2 11>                        |
+-----+-----+------+------------+-----------------------------------+
|  70 |     |  39  | SD_B0_04   | <gpio3 17>                        |
+-----+-----+------+------------+-----------------------------------+
|  71 | 11  |   6  | B0_10      | <gpio2 10>                        |
+-----+-----+------+------------+-----------------------------------+
|  73 | 10  |  45  | B0_09      | <gpio2 09>  / UART3 RX            |
+-----+-----+------+------------+-----------------------------------+

MMOD = Physical Micromod pin number
MMC = Zephyr micromod_header connector pin number
Pin = Arduino Pin number
Pad ID = MIMXRT1062 pad id
Usage = Some usages of the pin

Programming and Debugging
*************************

Flashing
========

The Teensy 4.0 and Teensy 4.1 and Micromod ship with a dedicated bootloader
chip, which supports flashing using USB. This allows easy flashing of new
images, but does not support debugging the device.

#. Build the Zephyr kernel and the :zephyr:code-sample:`blinky` sample application.

.. tabs::

   .. group-tab:: Teensy 4.0

      .. zephyr-app-commands::
         :zephyr-app: samples/basic/blinky
         :board: teensy40
         :goals: build
         :compact:

   .. group-tab:: Teensy 4.1

      .. zephyr-app-commands::
         :zephyr-app: samples/basic/blinky
         :board: teensy41
         :goals: build
         :compact:

   .. group-tab:: Teensy Micromod

      .. zephyr-app-commands::
         :zephyr-app: samples/basic/blinky
         :board: teensymm
         :goals: build
         :compact:

#. Connect the board to your host computer using USB.

#. Tap the reset button to enter bootloader mode.
   Red LED blinks.

#. Flash the image.

.. tabs::

   .. group-tab:: Teensy 4.0

      .. zephyr-app-commands::
         :zephyr-app: samples/basic/blinky
         :board: teensy40
         :goals: flash
         :compact:

   .. group-tab:: Teensy 4.1

      .. zephyr-app-commands::
         :zephyr-app: samples/basic/blinky
         :board: teensy41
         :goals: flash
         :compact:

   .. group-tab:: Teensy Micromod

      .. zephyr-app-commands::
         :zephyr-app: samples/basic/blinky
         :board: teensymm
         :goals: flash
         :compact:


#. You should see the orange LED blink.

Configuring a Console
=====================

.. tabs::

   .. group-tab:: UART-Console

      By default console output is mapped to teensy pins 0 (RX1) and 1 (TX1). Connect a usb-to-serial adapter
      to use this serial console. Use the following settings with your serial terminal of choice (minicom, putty,
      etc.):

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

   .. group-tab:: USB-Console

      By mapping the console output to USB, a usb-to-serial adapter is no longer required.
      Utilizing the :ref:`snippet-cdc-acm-console` and a config option will enable this feature.

      #. If application code doesn´t enable USB device support, this must be done via Kconfig option.

         .. code-block:: kconfig

            CONFIG_USB_DEVICE_INITIALIZE_AT_BOOT=y

      #. Build application including the snippet.

         .. zephyr-app-commands::
            :zephyr-app: samples/basic/blinky
            :snippets: cdc-acm-console
            :board: teensy41
            :goals: flash
            :compact:

      #. After application startup a serial device named like
         ``tty.usbmodem14203`` should appear on your host computer.
         You can use e.g. ``Serial Monitor`` plugin for VScode to monitor.

References
**********

.. target-notes::

.. _Teensy 4.0 Website:
   https://www.pjrc.com/store/teensy40.html

.. _Teensy 4.1 Website:
   https://www.pjrc.com/store/teensy41.html

.. _Teensy 4.1 Ethernet Kit:
   https://www.pjrc.com/store/ethernet_kit.html

.. _Teensy Micromod Website:
   https://www.sparkfun.com/sparkfun-micromod-teensy-processor.html

.. _i.MX RT1060 Datasheet:
   https://www.nxp.com/docs/en/nxp/data-sheets/IMXRT1060CEC.pdf
