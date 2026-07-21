.. zephyr:board:: blackpill_h523ce

Overview
********

The WeAct STM32H523CE Core Board is a low-cost and bare-bone development
board made in the famous "blackpill" package. It features the STM32H523CE, a
high-performance microcontroller based on an Arm Cortex-M33 processor.

Hardware
********

The STM32H523CE-based Black Pill board provides the following hardware
components:

- STM32H523CE in 48-pin package
- ARM 32-bit Cortex-M33 CPU with FPU
- 250 MHz max CPU frequency
- VDD from 1.7 V to 3.6 V
- 512 KB Flash
- 274 KB SRAM
- TIM
- ADC
- USART
- I2C
- SPI
- USB FS
- FDCAN
- RTC

Supported Features
******************

.. zephyr:board-supported-hw::

Connections and IOs
*******************

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1 TX/RX : PA9 / PA10
- I2C1 SCL/SDA : PB6 / PB7
- SPI1 CS/SCK/MISO/MOSI : PA4 / PA5 / PA6 / PA7 (routed to footprint for external flash)
- PWM_3_CH3 : PB0
- PWM_3_CH4 : PB1
- ADC_1 : PA1
- USER_PB : PA0
- USER_LED : PC13

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

There are 2 main entry points for flashing STM32H5X SoCs: one using the ROM
bootloader, and another by using the SWD debug port (which requires additional
hardware). Flashing using the ROM bootloader requires a special activation
pattern, which can be triggered by using the BOOT0 pin.

Flashing
********

Installing dfu-util
-------------------

It is recommended to use at least v0.8 of `dfu-util <https://dfu-util.sourceforge.net/>`_. The package available in
Debian/Ubuntu can be quite old, so you might have to build dfu-util from source.

There is also a Windows version which works, but you may have to install the
right USB drivers with a tool like `Zadig <https://zadig.akeo.ie/>`_.

Flashing an Application
-----------------------

Connect a USB-C cable and the board should power ON. Force the board into DFU mode
by keeping the BOOT0 switch pressed while pressing and releasing the NRST switch.

The dfu-util runner is supported on this board and so a sample can be built and
tested easily.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: blackpill_h523ce
   :goals: build flash
   :gen-args: -DCONFIG_BOOT_DELAY=5000

Debugging
=========

The board can be debugged by installing the included 100 mil (0.1 inch) header,
and attaching an SWD debugger to the 3V3 (3.3V), GND, SCK, and DIO
pins on that header.

References
**********

.. target-notes::

.. _WeAct Github:
   https://github.com/WeActStudio/WeActStudio.STM32H523CoreBoard/tree/master

.. _STM32H523CE website:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h523ce.html

.. _STM32H523CE reference manual:
   https://www.st.com/resource/en/reference_manual/rm0481-stm32h52333xx-stm32h56263xx-and-stm32h573xx-armbased-32bit-mcus-stmicroelectronics.pdf
