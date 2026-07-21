.. zephyr:board:: samc21n_xpro

Overview
********

The SAM C21N Xplained Pro evaluation kit is ideal for evaluation and
prototyping with the SAM C21N Cortex®-M0+ processor-based
microcontrollers. The kit includes Atmel’s Embedded Debugger (EDBG),
which provides a full debug interface without the need for additional
hardware.

Hardware
********

- SAMC21N18A ARM Cortex-M0+ processor at 48 MHz
- 32.768 kHz crystal oscillator
- 256 KiB flash memory, 32 KiB of RAM, 8KB RRW flash
- One yellow user LED
- One mechanical user push button
- One reset button
- One QTouch® button
- On-board USB based EDBG unit with serial console
- Two CAN transceivers

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

The SAM C21N Xplained Pro evaluation kit has 4 GPIO controllers. These
controllers are responsible for pin muxing, input/output, pull-up, etc.

For more details please refer to `SAM C21 Family Datasheet`_ and the `SAM C21N
Xplained Pro Schematic`_.

Default Zephyr Peripheral Mapping:
----------------------------------
- ADC0             : PB09
- ADC1             : PA08
- CAN0 TX          : PA24
- CAN0 RX          : PA25
- CAN1 TX          : PB14
- CAN1 RX          : PB15
- SERCOM0 USART TX : PB24
- SERCOM0 USART RX : PB25
- SERCOM1 I2C SDA  : PA16
- SERCOM1 I2C SCL  : PA17
- SERCOM2 USART TX : PA12
- SERCOM2 USART RX : PA13
- SERCOM4 USART TX : PB10
- SERCOM4 USART RX : PB11
- SERCOM5 SPI MISO : PB00
- SERCOM5 SPI MOSI : PB02
- SERCOM5 SPI SCK  : PB01
- GPIO/PWM LED0    : PC05

System Clock
============

The SAMC21 MCU is configured to use the 32.768 kHz internal oscillator
with the on-chip internal oscillator generating the 48 MHz system clock.

Serial Port
===========

The SAMC21 MCU has eight SERCOM based USARTs with three configured as USARTs in
this BSP. SERCOM4 is the default Zephyr console.

- SERCOM0 9600 8n1
- SERCOM2 115200 8n1
- SERCOM4 115200 8n1 connected to the onboard Atmel Embedded Debugger (EDBG)

PWM
===

The SAMC21 MCU has 3 TCC based PWM units with up to 4 outputs each and a period
of 24 bits or 16 bits.  If :code:`CONFIG_PWM_SAM0_TCC` is enabled then LED0 is
driven by TCC2 instead of by GPIO.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The SAM C21N Xplained Pro comes with a Atmel Embedded Debugger (EDBG). This
provides a debug interface to the SAMC21 chip and is supported by
OpenOCD.

Flashing
========

#. Build the Zephyr kernel and the ``hello_world`` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: samc21n_xpro
      :goals: build
      :compact:

#. Connect the SAM C21N Xplained Pro to your host computer using the USB debug
   port.

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyACM0`. For example:

   .. code-block:: console

      $ minicom -D /dev/ttyACM0 -o

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

#. To flash an image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: samc21n_xpro
      :goals: flash
      :compact:

   You should see "Hello World! samc21n_xpro" in your terminal.

References
**********

.. target-notes::

.. _Microchip website:
    https://www.microchip.com/en-us/development-tool/ATSAMC21N-XPRO

.. _SAM C21 Family Datasheet:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MCU32/ProductDocuments/DataSheets/SAM-C20-C21-Family-Data-Sheet-DS60001479J.pdf

.. _SAM C21N Xplained Pro Schematic:
    https://ww1.microchip.com/downloads/en/DeviceDoc/ATSAMC21N_Xplained_Pro_Design_Files.zip
