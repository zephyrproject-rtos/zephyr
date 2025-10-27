.. zephyr:board:: weact_stm32wb55_core

Overview
********

The WeAct Studio STM32WB55 Core Board is a compact multi-protocol wireless and
ultra-low-power development board embedding a powerful radio compliant with the
Bluetooth |reg| Low Energy (BLE) SIG specification v5.0 and with IEEE 802.15.4-2011.

- STM32WB55CGU6 microcontroller in UFQFPN48 package
- 2.4 GHz RF transceiver supporting Bluetooth |reg| specification v5.0 and
  IEEE 802.15.4-2011 PHY and MAC
- Dedicated Arm |reg| 32-bit Cortex |reg| M0+ CPU for real-time Radio layer
- One user LED (Blue)
- One BOOT button
- One RESET button
- USB Type-C connector
- Onboard 3.3V LDO regulator (ME6231A33M3G)
- Integrated PCB antenna with 2.4GHz RF filter
- 32 MHz HSE crystal oscillator
- 32.768 kHz LSE crystal oscillator for RTC
- 2.54mm pitch pin headers (P1: 20 pins, P2: 15 pins)
- 4-pin SWD debug header

Hardware
********

STM32WB55CGU6 is an ultra-low-power dual core Arm Cortex-M4 MCU 64 MHz, Cortex-M0+ 32MHz
with 1 Mbyte of Flash memory, Bluetooth 5, 802.15.4, USB, AES-256 SoC and
provides the following hardware capabilities:

- Ultra-low-power with FlexPowerControl (down to 600 nA Standby mode with RTC and 32KB RAM)
- Core: ARM |reg| 32-bit Cortex |reg|-M4 CPU with FPU, frequency up to 64 MHz
- Radio:

  - 2.4GHz
  - RF transceiver supporting Bluetooth |reg| 5 specification, IEEE 802.15.4-2011 PHY and MAC,
    supporting Thread and ZigBee |reg| 3.0
  - RX Sensitivity: -96 dBm (Bluetooth |reg| Low Energy at 1 Mbps), -100 dBm (802.15.4)
  - Programmable output power up to +6 dBm with 1 dB steps
  - Integrated balun to reduce BOM
  - Support for 2 Mbps
  - Dedicated Arm |reg| 32-bit Cortex |reg| M0+ CPU for real-time Radio layer
  - Accurate RSSI to enable power control

- Clock Sources:

  - 32 MHz crystal oscillator with integrated trimming capacitors (Radio and CPU clock)
  - 32 kHz crystal oscillator for RTC (LSE)
  - Internal low-power 32 kHz RC
  - Internal multispeed 100 kHz to 48 MHz oscillator, auto-trimmed by LSE
  - 2 PLLs for system clock, USB, and ADC

- RTC with HW calendar, alarms and calibration
- Up to 24 capacitive sensing channels
- 16x timers:

  - 2x 16-bit advanced motor-control
  - 2x 32-bit and 5x 16-bit general purpose
  - 2x 16-bit basic
  - 2x low-power 16-bit timers (available in Stop mode)
  - 2x watchdogs
  - SysTick timer

- Memories

  - 1 MB Flash, 2 banks read-while-write
  - Up to 256 KB of SRAM

- Rich analog peripherals

  - 12-bit ADC 4.26 Msps, up to 16-bit with hardware oversampling
  - 2x ultra-low-power comparator

SMBus/PMBus)
  - 1x USB 2.0 FS device, crystal-less, BCD and LPM

- Security and ID

  - 3x hardware encryption AES maximum 256-bit
  - HW public key authority (PKA)
  - True random number generator (RNG)
  - 96-bit unique ID

More information about STM32WB55CG can be found here:

- `STM32WB55CG on www.st.com`_
- `STM32WB55 datasheet`_
- `STM32WB55 reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Bluetooth and STM32WB Copro Wireless Binaries
==============================================

**IMPORTANT**: The WeAct STM32WB55 Core Board ships with a "Full Stack" wireless
binary pre-installed on the Cortex-M0+ coprocessor. This binary is **NOT compatible**
with Zephyr and must be replaced with an "HCI Layer" binary before using Bluetooth
functionality.

To operate Bluetooth on the WeAct STM32WB55 Core Board, the Cortex-M0+ core must
be flashed with a valid STM32WB Coprocessor HCI binary. These binaries are delivered
in STM32WB Cube packages, under ``Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x/``

For this board, the extended HCI binary has been tested: ``stm32wb5x_BLE_HCILayer_extended_fw.bin``

For compatibility information with the various versions of these binaries,
please check :module_file:`hal_stm32:lib/stm32wb/README.rst`.

Flashing the Wireless Coprocessor Binary
-----------------------------------------

The Cortex-M0+ wireless coprocessor binary **requires an external debug probe** and
STM32CubeProgrammer to flash. DFU mode cannot be used for the M0+ coprocessor.
Connect an external debug probe (ST-LINK/V2 or J-Link) to the 4-pin SWD
header (P3) and use STM32CubeProgrammer to program the binary at the appropriate
memory address.

The install address for STM32WB5xxG (1M variant) varies by binary type (v1.23.0):

- BLE_HCILayer_extended: 0x080DB000
- BLE_HCILayer: 0x080E1000

FUS (Firmware Upgrade Services) install address (v2.1.0):

- FUS v2.1.0: 0x080EE000


See the `STM32 Wireless Coprocessor Binary Table`_ for complete address information.

Connections and IOs
===================

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1 TX/RX : PA9/PA10
- LPUART_1 TX/RX : PA2/PA3 (with CTS: PA6, RTS: PB1)
- I2C_1_SCL : PB8
- I2C_1_SDA : PB9
- SPI_1_NSS : PA4
- SPI_1_SCK : PA5
- SPI_1_MISO : PB4
- SPI_1_MOSI : PB5
- PWM_1_CH1 : PA8
- PWM_2_CH1 : PA15
- ADC_1_IN5 : PA0
- ADC_1_IN6 : PA1
- USER_LED (Blue) : PB2
- BOOT_BUTTON : PE4
- USB_DP : PA12
- USB_DM : PA11

System Clock
------------

The WeAct STM32WB55 Core Board system clock is driven by a 32 MHz HSE crystal
oscillator. By default, the system clock is configured to run at 64 MHz using
the PLL with HSE as the source.

Power Supply
------------

The board includes an ME6231A33M3G 3.3V LDO regulator that accepts input voltage
from 3.3V to 5.5V. Power can be supplied via:

- USB Type-C connector (5V)
- VDD5V pins on header P1
- VDD33 pins on header P2 (3.3V regulated supply)

Serial Port
-----------

The board has 1 USART and 1 LPUART. The Zephyr console output is assigned to
USART1 (PA9/PA10). Default settings are 115200 8N1.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The WeAct STM32WB55 Core Board does not include an onboard debugger.
The Cortex-M4 can be flashed via DFU mode without an external probe, however
the Cortex-M0+ coprocessor requires an external debug probe (ST-LINK/V2 or J-Link)
connected to the 4-pin SWD header (P3).

Flashing
========

Installing dfu-util
-------------------

It is recommended to use at least v0.8 of `dfu-util`_. The package available in
debian/ubuntu can be quite old, so you might have to build dfu-util from source.
There is also a Windows version which works, but you may have to install the
right USB drivers with a tool like `Zadig`_.

Flashing an Application
-----------------------

Connect a USB Type-C cable and the board should power ON. Force the board into DFU mode
by keeping the BOOT button pressed while pressing and releasing the RESET button.

The dfu-util runner is supported on this board and so a sample can be built and
tested easily.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: weact_stm32wb55_core
   :goals: build flash

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: weact_stm32wb55_core
   :goals: build flash

Alternatively, if you have an external debug probe (ST-LINK/V2 or J-Link)
connected to the SWD header (P3), STM32CubeProgrammer can also be used to flash the
Cortex-M4 application using the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner stm32cubeprogrammer

Debugging
=========

You need an external debugger connected to the SWD header (P3) to debug
applications on this board. You can then debug an application in the usual way.
Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: weact_stm32wb55_core
   :maybe-skip-config:
   :goals: debug

.. _WeAct Studio GitHub:
   https://github.com/WeActStudio/WeActStudio.STM32WB55CoreBoard

.. _STM32WB55CG on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32wb55cg.html

.. _STM32WB55 datasheet:
   https://www.st.com/resource/en/datasheet/stm32wb55cg.pdf

.. _STM32WB55 reference manual:
   https://www.st.com/resource/en/reference_manual/dm00318631.pdf

.. _dfu-util:
   https://dfu-util.sourceforge.net/

.. _Zadig:
   https://zadig.akeo.ie/

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html

.. _STM32 Wireless Coprocessor Binary Table:
   https://github.com/STMicroelectronics/STM32CubeWB/blob/master/Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x/Release_Notes.html
