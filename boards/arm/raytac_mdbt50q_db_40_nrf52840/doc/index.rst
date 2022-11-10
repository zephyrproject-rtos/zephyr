.. _raytac_mdbt50q_db_40:

MDBT50Q-DB-40
###########

Overview
********

The MDBT50Q-DB-40 hardware provides support for the
Nordic Semiconductor nRF52840 ARM Cortex-M4F CPU and the following devices:

* ADC (Analog to Digital Converter)
* CLOCK
* FLASH
* GPIO (General Purpose Input Output)
* I2C (Inter-Integrated Circuit)
* MPU (Memory Protection Unit)
* NVIC (Nested Vectored Interrupt Controller)
* PWM (Pulse Width Modulation)
* RADIO (Bluetooth Low Energy and 802.15.4)
* RTC (nRF RTC System Clock)
* Segger RTT (RTT Console)
* SPI (Serial Peripheral Interface)
* UART (Universal asynchronous receiver-transmitter)
* USB (Universal Serial Bus)
* WDT (Watchdog Timer)

.. figure:: img/mdbt50q_db_40.jpg
     :width: 442px
     :align: center
     :alt: MDBT50Q-DB-40

     MDBT50Q-DB-40 (Credit: Raytac Corporation)

More information about the board can be found at the `MDBT50Q-DB-40 website`_.
The `MDBT50Q-DB-40 Specification`_ contains the demo board's datasheet.
The `MDBT50Q-DB-40 Schematic`_ contains the demo board's schematic.

Hardware
********
- Module Demo Board build by MDBT50Q-1MV2
- Nordic nRF52840 SoC Solution Version: 2
- A recommnded 3rd-party module by Nordic Semiconductor.
- BT5.2&BT5.1&BT5 Bluetooth Specification Cerified
- Supposts BT5 Long Range Features
- Cerifications: FCC, IC, CE, Telec(MIC), KC, SRRC, NCC, RCM, WPC
- 32-bit ARM Cortx M4F CPU
- 1MB Flash Memory/256kB RAM
- RoHs & Reach Compiant.
- 48 GPIO
- Chip Antenna
- Interfaces: SPI, UART, I2C, I2S, PWM, ADC, NFC, and USB
- 3 User LEDs
- 4 User buttons
- 1 Mini USB connector for power supply and USB communication
- SWD connector for FW programing
- J-Link interface for FW programing
- UART interface for UART communication

Supported Features
==================

The raytac_mdbt50q_db_40 board configuration supports the following
hardware features:

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
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| RADIO     | on-chip    | Bluetooth,           |
|           |            | ieee802154           |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| RTT       | Segger     | console              |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| QSPI(M)   | on-chip    | qspi                 |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| USB       | on-chip    | usb                  |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Other hardware features have not been enabled yet for this board.
See `MDBT50Q-DB-40 website`_ and `MDBT50Q-DB-40 Specification`_
for a complete list of Raytac MDBT50Q-DB-40 board hardware features.

Connections and IOs
===================

LED
---

* LED1 (green) = P0.13
* LED2 (red) = P0.14
* LED3 (blue) = P0.15


Push buttons
------------

* BUTTON1 = SW1 = P0.11
* BUTTON2 = SW2 = P0.12
* BUTTON3 = SW3 = P0.24
* BUTTON4 = SW4 = P0.25

UART
----
* RXD = P0.08
* TXD = P0.06
* RTS = P0.05
* CTS = P0.07

Programming and Debugging
*************************

Applications for the ``raytac_mdbt50q_db_40`` board configuration can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Flashing
========

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`. Then build and flash
applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :ref:`blinky` application.

Then build and flash the application in the usual way.

On Windows, go to the NCS directory and execute the following commands.
The FW will be program to MDBT50Q-DB-40 demo board.

.. code-block:: console

   west build -b raytac_mdbt50q_db_40 ./zephyr/sample/basic/blinky --build-dir build_raytac_mdbt50q_db_40
   west flash --build-dir build_raytac_mdbt50q_db_40
   

Debugging
=========

Refer to the :ref:`nordic_segger` page to learn about debugging Nordic boards with a
Segger IC.


Testing the LEDs and buttons in the MDBT50Q-DB-40
***********************************************

There are 2 samples that allow you to test that the buttons (switches) and LEDs on
the board are working properly with Zephyr:

.. code-block:: console

   zephyr/samples/basic/blinky
   zephyr/samples/basic/button

You can build and flash the examples to make sure Zephyr is running correctly on
your board. The button and LED definitions can be found in
:zephyr_file:`boards/arm/raytac_mdbt50q_db_40/raytac_mdbt50q_db_40.dts`.

Using UART1
***********

The following approach can be used when an application needs to use
more than one UART for connecting peripheral devices:

1. Add devicetree overlay file to the main directory of your application:

   .. code-block:: devicetree

      &pinctrl {
         uart1_default: uart1_default {
            group1 {
               psels = <NRF_PSEL(UART_TX, 0, 14)>,
                       <NRF_PSEL(UART_RX, 0, 16)>;
            };
         };
         /* required if CONFIG_PM_DEVICE=y */
         uart1_sleep: uart1_sleep {
            group1 {
               psels = <NRF_PSEL(UART_TX, 0, 14)>,
                       <NRF_PSEL(UART_RX, 0, 16)>;
               low-power-enable;
            };
         };
      };

      &uart1 {
        compatible = "nordic,nrf-uarte";
        current-speed = <115200>;
        status = "okay";
        pinctrl-0 = <&uart1_default>;
        pinctrl-1 = <&uart1_sleep>;
        pinctrl-names = "default", "sleep";
      };

   In the overlay file above, pin P0.16 is used for RX and P0.14 is used for TX

2. Use the UART1 as ``DEVICE_DT_GET(DT_NODELABEL(uart1))``

See :ref:`set-devicetree-overlays` for further details.

Selecting the pins
==================

Pins can be configured in the board pinctrl file. To see the available mappings,
open the `nRF52840 Product Specification`_, chapter 7 'Hardware and Layout'.
In the table 7.1.1 'aQFN73 ball assignments' select the pins marked
'General purpose I/O'.  Note that pins marked as 'low frequency I/O only' can only be used
in under-10KHz applications. They are not suitable for 115200 speed of UART.

References
**********

.. target-notes::

.. _MDBT50Q-DB-40 website:
	https://www.raytac.com/product/ins.php?index_id=81
.. _MDBT50Q-DB-40 Specification:
	https://www.raytac.com/download/index.php?index_id=43
.. _MDBT50Q-DB-40 Schematic:
	https://www.raytac.com/upload/catalog_b/134ade06b5db3dd5803d27c5b17f22f3.jpg
.. _J-Link Software and documentation pack:
	https://www.segger.com/jlink-software.html
.. _nRF52840 Product Specification:
	http://infocenter.nordicsemi.com/pdf/nRF52840_PS_v1.0.pdf
