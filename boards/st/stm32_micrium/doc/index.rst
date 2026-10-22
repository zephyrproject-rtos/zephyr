.. zephyr:board:: stm32_micrium

Overview
********

The STM32-MICRIUM evaluation board is a development platform for the
STM32F107VCT6 microcontroller. It exposes the Ethernet MAC, USB OTG
full speed, CAN, an SD card socket and an RS-232 port of the connectivity
line STM32F107, together with an on-board SEGGER J-Link debugger.

More information about the board can be found in the `STM32-MICRIUM User Manual`_.

Hardware
********

- STM32F107VCT6 microcontroller in an LQFP100 package
- Arm® 32-bit Cortex®-M3 CPU
- 256 KB of Flash
- 64 KB of SRAM
- 25 MHz crystal oscillator
- 32.768 kHz crystal oscillator for the RTC
- 10/100 Ethernet with a TI DP83848CVV PHY in MII mode
- USB OTG full speed on a Micro-AB connector with an STMPS2141 VBUS power switch
- One CAN channel with an SN65HVD230 transceiver
- SD card socket
- STLM75 temperature sensor
- RS-232 with hardware flow control on a DB-9 connector
- Three user LEDs (green, orange and red)
- 46-pin extension connector and a prototyping area
- On-board SEGGER J-Link (JTAG and SWD) on a mini-B USB connector
- Powered from the J-Link USB connector or from a 5V screw terminal

For more details on the microcontroller refer to the `STM32F107VC on
www.st.com`_ and the `STM32F105xx/107xx reference manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::


Default Zephyr Peripheral Mapping:
----------------------------------

- USART2 TX/RX/RTS/CTS : PD5/PD6/PD4/PD3 (RS-232 connector CN7)
- I2C1 SCL/SDA : PB6/PB7 (STLM75 at address 0x48, OS/INT on PB5)
- SPI1 SCK/MISO/MOSI/CS : PA5/PA6/PA7/PA8 (SD card socket)
- SD card detect : PE6
- CAN1 RX/TX : PD0/PD1
- Ethernet MII : PA0, PA1, PA3, PB8, PB10, PB11, PB12, PB13, PC2, PC3, PD8 to PD12
- Ethernet MDC/MDIO : PC1/PA2
- Ethernet PHY interrupt : PE5
- USB OTG FS DM/DP/VBUS/ID : PA11/PA12/PA9/PA10
- LD1 (green) : PD13
- LD2 (orange) : PD14
- LD3 (red) : PD15

System Clock
------------

The system clock is driven by the 25MHz crystal through PLL2 and the main PLL: the HSE is
divided by 5 and multiplied by 8 in PLL2 (40 MHz), then divided by 5 and multiplied by 9 in the
main PLL, giving a 72 MHz system clock. The OTG FS clock is the PLL VCO output divided by 3
(48 MHz). The RTC runs from the 32.768kHz crystal.

Ethernet
--------

The DP83848 PHY is strapped to address 1 and drives the MAC in MII mode from its own 25 MHz
crystal.

USB
---

The board cannot be powered from the OTG connector. The STMPS2141 power switch on PE1 supplies
VBUS in host mode; it is not driven by Zephyr, which uses the controller in device mode only.

CAN
---

The CAN bus is available on CN3 pins 28 (CAN_L) and 30 (CAN_H). The on-board 120 ohm
termination resistor is connected by fitting JP1 (not fitted by default).

Boot options
------------

JP4 (BOOT0) and JP3 (BOOT1) select the boot source. In the default setting JP4 selects the user
flash, which is what Zephyr expects.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The board integrates a SEGGER J-Link debug probe. Its SWDIO and SWCLK lines are always wired to
the STM32F107VCT6; the remaining JTAG signals (TDI, TDO and TRST) pass through solder bridges
SB1 to SB3, which are closed by default, so both JTAG and SWD are available. The Zephyr runners
use SWD, which works in either bridge configuration. With JP2 open (default) the on-board probe
is used; closing JP2 disconnects it so that an external probe can be attached to the JTAG/SWD
signals on CN3. PA15, PB3 and PB4 are shared between the JTAG port and CN3, so using them as
GPIOs requires opening SB1 to SB3 and disabling JTAG through the ``swj-cfg`` pinctrl property.

Applications for the ``stm32_micrium`` board can be built and flashed in the usual way (see
:ref:`build_an_application` and :ref:`application_run` for more details).

Flashing
========

The default runner is `J-Link`_. Connect the J-Link mini-B USB connector (CN5) to the host,
which also powers the board when JP5 is in its default position.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32_micrium
   :goals: build flash

You will see the green LED blinking every second.

Debugging
=========

The console is available on the RS-232 connector CN7 at 115200 bauds. The RTS and CTS lines
are wired to the connector and routed to USART2, but hardware flow control is off by default so
that the console works with a three-wire cable. Enable flow control with an overlay:

.. code-block:: devicetree

   &usart2 {
       hw-flow-control;
   };

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32_micrium
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _STM32-MICRIUM User Manual:
   https://web.archive.org/web/20170704164211/http://www.st.com/content/ccc/resource/technical/document/user_manual/9e/35/1b/4b/c3/3f/46/33/CD00246066.pdf/files/CD00246066.pdf/jcr:content/translations/en.CD00246066.pdf

.. _STM32F107VC on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32f107vc.html

.. _STM32F105xx/107xx reference manual:
   https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _J-Link:
   https://www.segger.com/products/debug-probes/j-link/
