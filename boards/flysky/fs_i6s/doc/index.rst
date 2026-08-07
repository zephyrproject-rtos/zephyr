.. zephyr:board:: fs_i6s

Overview
********
The FlySky FS-i6s is a 10-channel 2.4GHz remote controller transmitter commonly
used for RC multirotors and fixed-wing aircraft. The internal hardware is driven
by an ST STM32F072VB microcontroller. It features a capacitive
touchscreen, physical gimbals, switches, push buttons, and an integrated RF
module.

Hardware
********

- STM32F072VB in LQFP100 package
- ARM® 32-bit Cortex®-M0 CPU
- 48 MHz max CPU frequency
- VDD from 2.0 V to 3.6 V
- 128 KB Flash
- 16 KB SRAM
- GPIO with external interrupt capability
- 12-bit ADC with 39 channels
- 12-bit D/A converters
- RTC
- General Purpose Timers (12)
- USART/UART (4)
- I2C (2)
- SPI (2)
- CAN
- USB 2.0 full speed interface
- DMA Controller

Supported Features
==================

.. zephyr:board-supported-hw::

Default Zephyr Peripheral Mapping:
==================================

LEDs and Audio
--------------
* **LCD Backlight:** ``PF3`` (Screen backlight control)
* **Left Power LED:** ``PD10`` (Blue LED under the left power button)
* **Right Power LED:** ``PD11`` (Blue LED under the right power button)
* **Buzzer:** ``PA8`` (Piezo buzzer, can be driven by ``TIM1_CH1``)

Buttons and Power Management
----------------------------
.. note::
   The two physical power buttons must be pressed simultaneously to trigger the associated GPIO.
   Single button presses cannot be read independently.

* **Power Buttons:** ``PB14`` (Both power buttons pressed together)
* **Rear Left Button:** ``PA9`` (Tactile push button on the back)
* **Rear Right Button:** ``PA10`` (Tactile push button on the back)
* **Powerdown Trigger:** ``PB15`` (Used to trigger system power off)

Analog Inputs (ADC)
-------------------
All gimbals, potentiometers, and toggle switches are read using the STM32's
ADC multiplexer.

* **Right Stick L/R:** ``PA0``, ``ADC_CH0`` (Right gimbal horizontal axis)
* **Right Stick U/D:** ``PA1``, ``ADC_CH1`` (Right gimbal vertical axis)
* **Left Stick U/D:** ``PA2``, ``ADC_CH2`` (Left gimbal vertical axis, throttle)
* **Left Stick L/R:** ``PA3``, ``ADC_CH3`` (Left gimbal horizontal axis)
* **Switch A (Left):** ``PA4``, ``ADC_CH4`` (2-way toggle switch)
* **Switch B (Left):** ``PA5``, ``ADC_CH5`` (3-way toggle switch)
* **Jog Wheel Left:** ``PA6``, ``ADC_CH6`` (Left proportional shoulder wheel)
* **Jog Wheel Right:** ``PA7``, ``ADC_CH7`` (Right proportional shoulder wheel)
* **Switch C (Right):** ``PB0``, ``ADC_CH8`` (3-way toggle switch)
* **Switch D (Right):** ``PB1``, ``ADC_CH9`` (2-way toggle switch)
* **Battery Voltage:** ``PC0``, ``ADC_CH10`` (Requires scaling, Divider:
  R9=10kΩ, R10=5.1kΩ to GND)

Display (Sitronix ST7567)
-------------------------
The LCD uses a Sitronix ST7567 controller and is wired to the MCU using an
8-bit parallel interface.

* **LCD_D0 - D7:** ``PE0`` - ``PE7`` (8-bit Data Bus)
* **LCD_RS:** ``PB3`` (Register Select / Data Command)
* **LCD_RST:** ``PB4`` (Hardware Reset)
* **LCD_RW:** ``PB5`` (Read/Write)
* **LCD_RD:** ``PD7`` (Read enable)
* **LCD_CS:** ``PD2`` (Chip Select)

Touch Controller (Focaltech FT6236)
-----------------------------------
The capacitive touch overlay is driven by an FT6236 communicating over ``I2C1``.

* **TOUCH_SCL:** ``PB8`` (I2C1 Clock)
* **TOUCH_SDA:** ``PB9`` (I2C1 Data)
* **TOUCH_INT:** ``PC12`` (Interrupt out from FT6236)
* **TOUCH_RST:** ``PA15`` (Hardware Reset)

USART
-----

- **UART_1_TX:** ``PB6``
- **UART_1_RX:** ``PB7``

RF Module (For Reference Only, Not Used)
----------------------------------------
The internal RF module is labeled ``FS-HS060``. It contains an XL7105
transceiver and PA/LNA chips.

* **SPI Lines:** ``PE15`` (SDIO), ``PE13`` (SCK), ``PE12`` (SCS)
* **Control Lines:** ``PE11`` (RF1), ``PE10`` (RF0), ``PE8`` (RX-W),
  ``PE9`` (TX-W)
* **Interrupts:** ``PB2`` (GIO2), ``PE14`` (GIO1)

Programming and Debugging
*************************

.. zephyr:board-supported-runners::


SWD Interface
=============
The PCB features labeled test points for the Serial Wire Debug (SWD) interface,
making it easy to solder a header for an ST-Link or J-Link programmer. These
are also accessible via the small ``J4`` connector on the board:

* **SWDIO**
* **SWCLK**
* **NRST**
* **GND**

DFU Bootloader
==============
Because the STM32F072 has a built-in USB DFU bootloader, you can
flash Zephyr without a dedicated SWD programmer. Depending on your unit, the
internal flash may need its write protect bit turned off using ST-Link SWD.

To enter DFU mode:

1. Locate resistors ``R59`` and ``R60`` (connected to the ``BOOT1`` pin).
2. Short ``R60``.
3. Connect the batteries (or USB).
4. Press the two power buttons simultaneously.
5. The device should enumerate on your PC as an "STM32 BOOTLOADER" device,
   allowing you to flash the Zephyr ``.bin`` using ``dfu-util``.


Flashing an Application
-----------------------

The dfu-util runner is supported on this board and so a sample can be built and
tested easily.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: fs_i6s
   :goals: build flash


References
**********
* Hardware reverse-engineering and pinout source:
  `fishpepper.de - OpenGround
  <https://fishpepper.de/2016/09/15/openground-part-1-components-pinout/>`_
