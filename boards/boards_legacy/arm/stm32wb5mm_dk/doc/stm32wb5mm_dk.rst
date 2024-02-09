.. _stm32wb5mm_dk_discovery_kit:

ST STM32WB5MM-DK
################

Overview
********

The STM32WB5MM-DK Discovery kit is designed as a complete demonstration
and development platform for the STMicroelectronics STM32W5MMG module based
on the Arm |reg| Cortex |reg|-M4 and Arm |reg|  Cortex |reg|-M0+ cores.
The STM32 device is a multi-protocol wireless and ultra-low-power device
embedding a powerful and ultra-low-power radio compliant with the
Bluetooth |reg| Low Energy (BLE) SIG specification v5.2 and with
IEEE 802.15.4-2011.


STM32WB5MM-DK supports the following features:

* STM32WB5MMG (1-Mbyte Flash memory, 256-Kbyte SRAM)
    - Dual-core 32‑bit (Arm |reg| Cortex |reg|-M4 and M0+)
    - 2.4 GHz RF transceiver
    - 0.96-inch 128x64 OLED display
    - 128-Mbit Quad-SPI NOR Flash Memory
    - Temperature sensor
    - Accelerometer/gyroscope sensor
    - Time-of-Flight and gesture-detection sensor
    - Digital microphone
    - RGB LED
    - Infrared LED
    - 3 push-buttons (2 users and 1 reset) and 1 touch key button

* Board connectors:
    - STMod+
    - ARDUINO |reg| Uno V3 expansion connector
    - USB user with Micro-B connector
    - TAG10 10-pin footprint

* Flexible power-supply options:
    - ST-LINK/V2-1 USB connector,
    - 5 V delivered by:
        - ARDUINO |reg|,
        - external connector,
        - USB charger, or USB power

* On-board ST-LINK/V2-1 debugger/programmer with USB re-enumeration
    - Virtual COM port and debug port


.. image:: img/STM32WB5MM_DK.jpg
   :align: center
   :alt: STM32WB5MM-DK

More information about the board can be found in `STM32WB5MM-DK on www.st.com`_.

Hardware
********

STM32WB5MMG is an ultra-low-power and small form factor certified 2.4 GHz
wireless module. It supports Bluetooth |reg| Low Energy 5.4, Zigbee |reg| 3.0,
OpenThread, dynamic, and static concurrent modes, and 802.15.4 proprietary
protocols.

Based on the STMicroelectronics STM32WB55VGY wireless microcontroller,
STM32WB5MMG provides best-in-class RF performance thanks to its high
receiver sensitivity and output power signal. Its low-power features
enable extended battery life, small coin-cell batteries, and energy harvesting.

- Ultra-low-power with FlexPowerControl
- Core: ARM |reg| 32-bit Cortex |reg|-M4 CPU with FPU
- Radio:

  - 2.4GHz
  - RF transceiver supporting:

    - Bluetooth |reg| 5.4 specification,
    - IEEE 802.15.4-2011 PHY and MAC,
    - Zigbee |reg| 3.0

  - RX sensitivity:

    - -96 dBm (Bluetooth |reg| Low Energy at 1 Mbps),
    - -100 dBm (802.15.4)

  - Programmable output power up to +6 dBm with 1 dB steps
  - Integrated balun to reduce BOM
  - Support for 2 Mbps
  - Support GATT caching
  - Support EATT (enhanced ATT)
  - Support advertising extension
  - Accurate RSSI to enable power control

- Clock Sources:

  - 32 MHz crystal oscillator with integrated
    trimming capacitors (Radio and CPU clock)
  - 32 kHz crystal oscillator for RTC (LSE)
  - Internal low-power 32 kHz (±5%) RC (LSI1)
  - Internal low-power 32 kHz (stability
    ±500 ppm) RC (LSI2)
  - Internal multispeed 100 kHz to 48 MHz
    oscillator, auto-trimmed by LSE (better than
    ±0.25% accuracy)
  - High speed internal 16 MHz factory
    trimmed RC (±1%)
  - 2x PLL for system clock, USB, SAI, ADC

More information about STM32WB55RG can be found here:

- `STM32WB5MM-DK on www.st.com`_
- `STM32WB5MMG datasheet`_

Supported Features
==================

The Zephyr STM32WB5MM-DK board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+


Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in the defconfig file:
``boards/arm/stm32wb5mm_dk/stm32wb5mm_dk_defconfig``

Bluetooth and compatibility with STM32WB Copro Wireless Binaries
================================================================

To operate bluetooth on STM32WB5MMG, Cortex-M0 core should be flashed with
a valid STM32WB Coprocessor binaries (either 'Full stack' or 'HCI Layer').
These binaries are delivered in STM32WB Cube packages, under
``Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x/``.

For compatibility information with the various versions of these binaries,
please check `modules/hal/stm32/lib/stm32wb/hci/README`_
in the ``hal_stm32`` repo.

Note that since STM32WB Cube package V1.13.2, `"full stack"` binaries are not
compatible anymore for a use in Zephyr and only `"HCI Only"` versions should be
used on the M0 side.

Connections and IOs
===================


Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- UART_1 TX/RX : PB7/PB6 ( Connected to ST-Link VCP)
- LPUART_1 TX/RX : PA3/PA2
- USB : PA11/PA12
- SWD : PA13/PA14

System Clock
------------

STM32WB5MMG System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by HSE clock at 32MHz.

Serial Port
-----------

STM32WB5MM-DK board has 2 (LP)U(S)ARTs. The Zephyr console output is assigned to USART1.
Default settings are ``115200 8N1``.


Programming and Debugging
*************************

Applications for the ``stm32wb5mm_dk`` board configuration can be built the
usual way (see :ref:`build_an_application`).

Flashing
========

STM32WB5MM-DK has an on-board ST-Link to flash and debug the firmware on the
module.


Flashing ``hello_world`` application to STM32WB5MM-DK
------------------------------------------------------

Connect the STM32WB5MM-DK to your host computer using the USB port (CN11).
Then build and flash an application. Here is an example for the ``hello_world``
application.

Run a serial host program to connect with your  STM32WB5MM-DK board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then first build and flash the application for the STM32WB5MM-DK board.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32wb5mm_dk
   :goals: build flash

Reset the board and you should see the following messages on the console:

.. code-block:: console

	Hello World! stm32w5mm_dk

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
`Hello_World`_  application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32wb5mm_dk
   :maybe-skip-config:
   :goals: debug

.. _STM32WB5MM-DK on www.st.com:
   https://www.st.com/en/evaluation-tools/stm32wb5mm-dk.html
.. _STM32WB5MMG datasheet:
   https://www.st.com/resource/en/datasheet/stm32wb5mmg.pdf
.. _modules/hal/stm32/lib/stm32wb/hci/README:
   https://github.com/zephyrproject-rtos/hal_stm32/blob/main/lib/stm32wb/hci/README
.. _Hello_World:
   https://docs.zephyrproject.org/latest/samples/hello_world/README.html
