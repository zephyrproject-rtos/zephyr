.. zephyr:board:: stm32h7_hdmi_board

Overview
********

`STM32H7 HDMI Board`_ integrates an STMEWH747XIH6 MCU, along with multiple external peripherals.
The PCB utilizes extended memory resources such as external SDRAM, eMMC, microSD card, NOR flash
and interfaces such as USB, Ethernet (with PoE+), HDMI, analog audio and CAN.

Key features include:

- STM32H747XIH6 MCU
- External SDRAM on SODIMM 144-pin
- MicroSD card slot
- 4 GB eMMC memory
- 2 Gb QSPI NOR flash
- 2x USB-A ports
- HDMI support
- DSI support
- 10/100Mbit Ethernet with PoE+
- Debug USB-C interface with Power Delivery support
- USB-C OTG support
- PMOD connector
- CAN and industrial standard GPIOs
- Analog audio interface
- 120 x 120 mm (4.72 x 4.72 inch) PCB

Hardware
********

The STM32H7 HDMI Board provides the following hardware:

- STM32H747XI MCU
- ARM Cortex-M7 + Cortex-M4 MCU
- Memory:

   - 1 MB RAM
   - 2 MB Flash
   - 2 Gbit QSPI NOR external flash memory
   - External SDRAM on SODIMM 144-pin
   - 4 GB eMMC memory
   - MicroSD card slot

- Communication interfaces:

   - 1x FDCAN
   - 1x I2C Fast Mode SMBus/PMBus support
   - 1x UART via USB-C Debug port
   - 1x USB-C OTG full-speed
   - 2x USB-A
   - Ethernet MAC (10/100 MBit/s) with PoE+
   - HDMI / DSI display
   - 4x industrial GPIOs with control up to 24V and read up to 58V
   - PMOD Connector:

      - 1x SPI (Type 2) / 1x USART (Type 3) selected via on board dip switch
      - 4x GPIOs on pins 7-10

- Other peripherals:

   - 1x RGB Led connected via GPIOs
   - 1x user button
   - Chrom-ART Accelerator (DMA2D), LCD-TFT controller, JPEG Codec
   - True Random Number Generator (RNG)
   - CRC calculation unit
   - Real-time clock

More information about the STM32H747XI can be found here:

- `STM32H747XI on www.st.com`_

Full component list for the board can be found here:

- `STM32H7 HDMI Board on Antmicro Designer`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Antmicro's STM32H7 HDMI Board uses the following default pin mappings for peripherals:

- I2C2 SCL/SDA : PH4/PH5
- I2C4 SCL/SDA : PD12/PB7
- QSPI CLK/NCS/IO10/IO11/IO12/IO13/IO20/IO21/IO22/IO23 : PF10/PG6/PF8/PF9/PF7/PD13/PH2/PH3/PG9/PG14
- SPI4 NSS/SCK/MOSI/MISO : PE4/PE2/PE6/PE5
- PMOD SPI5 NSS/SCK/MOSI/MISO : PI8/PD3/PJ10/PJ11
- PMOD USART2 CTS/TX/RX/RTS : PD3/PD5/PA3/PD4
- UART5 TX/RX : PB6/PB12
- PMOD GPIO 1/2/3/4 : PK3/PK4/PK5/PJ15
- ETH : PA7, PC1, PA2, PA1, PC4, PC5, PB11, PG13, PG12, PB2
- FDCAN2 RX/TX/SHDN/STB : PB5/PB13/PJ6/PJ7
- SDMMC (SD) CK/CMD/D0/D1/D2/D3/SD_SEN : PC12/PD2/PC8/PC9/PC10/PC11/PA8
- SDMMC (eMMC) CK/CMD/D0/D1/D2/D3/D4/D5/D6/D7/NRST/STRB : PD6/PA0/PB14/PB15/PG11/PB4/PB8/PB9/PC6/PC7/PK6/PK7
- USBA1 NRES/INT/GPX : PC13/PE3/PI14
- USBA2 NRES/INT/GPX : PJ8/PJ9/PI15
- USB OTG D-/D+/ID/VBUS : PA11/PA12/PA10/PA9
- I2S1 CK/SDI/SDO/LRCLK : PA5/PA6/PD7/PG10
- IGPIO O1/O2/I1/I2 : PB0/PB1/PJ3/PJ4
- LED R/G/B : PJ0/PJ1/PJ2
- BTN : PI12
- JTAG TMS/TCK/TDO/TDI : PA13/PA14/PB3/PA15

Serial Port
===========

The Zephyr console output is assigned to UART5, which is connected to USB-UART converter on USB PD port.

External Displays
=================

The STM32H7 HDMI Board is capable of connecting HDMI display or external DSI display over
DSI FMC connector. Both displays are connected to the same MIPI DSI lines, which means using
HDMI and DSI FMC at the same time is impossible.

Resources sharing
=================

The dual core nature of STM32H747 SoC requires sharing HW resources between the
two cores. This is done in 3 ways:

- **Compilation**: Clock configuration is only accessible to M7 core. M4 core only
  has access to bus clock activation and deactivation.
- **Static pre-compilation assignment**: Peripherals such as a UART are assigned in
  devicetree before compilation. The user must ensure peripherals are not assigned
  to both cores at the same time.
- **Run time protection**: Interrupt-controller and GPIO configurations could be
  accessed by both cores at run time. Accesses are protected by a hardware semaphore
  to avoid potential concurrent access issues.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``stm32h7_hdmi_board`` board should be built per core target,
using either ``stm32h7_hdmi_board/stm32h747xx/m7`` or ``stm32h7_hdmi_board/stm32h747xx/m4``
as the target.
See :ref:`build_an_application` for more information about application builds.

.. note::

   The FTDI4232HP chip used for flashing the board may require a newer version of OpenOCD
   than the one provided by Zephyr SDK.

Flashing
========

This board has a USB-JTAG interface and can be used with OpenOCD.

Connect the board to your host computer using USB PD port, then build and flash the application.

Flashing an application to STM32H747I M7 Core
---------------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32h7_hdmi_board/stm32h747xx/m7
   :goals: build flash

Run a serial host program to connect with the board:

.. code-block:: console

   $ picocom /dev/ttyUSB2 -b 115200


Similarly, you can build and flash samples on the M4 target. For this, please take care of
the resource sharing (UART port used for console for instance).

Here is an example for the :zephyr:code-sample:`hello_world` application on M4 core.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h7_hdmi_board/stm32h747xx/m4
   :goals: build flash

Debugging
=========

You can debug an application on Cortex M7 in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h7_hdmi_board/stm32h747xx/m7
   :maybe-skip-config:
   :goals: debug

Debugging a Zephyr application on Cortex M4 side with west is currently not available.

References
**********

.. target-notes::

.. _`STM32H7 HDMI Board`: https://openhardware.antmicro.com/boards/stm32h7-hdmi-board

.. _`STM32H7 HDMI Board on Antmicro Designer`: https://designer.antmicro.com/library/devices/stm32h7-hdmi-board

.. _`STM32H747XI on www.st.com`: https://www.st.com/en/microcontrollers-microprocessors/stm32h747xi.html
