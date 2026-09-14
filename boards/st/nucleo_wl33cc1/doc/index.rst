.. zephyr:board:: nucleo_wl33cc1

Overview
********

The NUCLEO-WL33CC1 board is based on the MB1801 mezzanine board and features an
MB2029 MCU RF board embedding an ultra-low-power STM32WL33CCV6
Arm® Cortex®-M0+ wireless microcontroller with an integrated sub-GHz
radio operating in the 826-958 MHz frequency band.

Key features:

- STM32WL33CCV6 (256 KB flash, 32 KB RAM, Cortex-M0+)
- Sub-GHz radio supporting OOK, ASK, 2(G)FSK, 4(G)FSK, D-BPSK, and DSSS modulations
- Three user LEDs (LD1 blue, LD2 green, LD3 red)
- Three user push-buttons
- On-board STLINK-V3EC debugger/programmer with USB VCP
- ARDUINO® Uno V3 and ST morpho expansion connectors (MB1801)

More information about the board can be found on the
`NUCLEO-WL33CC1 product page`_, in the board user manual (`UM3418`_), and in
the SoC reference manual (`RM0511`_).

Supported Features
******************

The following hardware features are currently supported:

- GPIO
- USART (console via the ST-LINK Virtual COM Port)

Not yet supported: the sub-GHz radio (MR_SubG), ADC, SPI, I2C, timers, RTC,
watchdog and low-power modes.

.. zephyr:board-supported-hw::

Connections and IOs
*******************

Default board configuration:

- USART1 TX/RX : PA1/PA15 (ST-LINK Virtual COM Port)
- LD1 (blue)   : PA14
- LD2 (green)  : PB4
- LD3 (red)    : PB5
- User button B1 : PA0
- User button B2 : PA11
- User button B3 : PB15

Programming and Debugging
*************************

Applications are flashed and debugged through the on-board STLINK-V3EC using
the ``stm32cubeprogrammer`` runner (default):

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_wl33cc1
   :goals: build flash

Connect a serial terminal to the ST-LINK VCP at 115200 baud to see console output.

References
**********

.. target-notes::

.. _NUCLEO-WL33CC1 product page:
   https://www.st.com/en/evaluation-tools/nucleo-wl33cc1.html

.. _RM0511:
   https://www.st.com/resource/en/reference_manual/rm0511-stm32wl30xx31xx33xx-armbased-wireless-mcus-with-subghz-radio-solution-stmicroelectronics.pdf

.. _UM3418:
   https://www.st.com/resource/en/user_manual/um3418-stm32wl33-nucleo64-boards-mb1801-and-mb2029-stmicroelectronics.pdf
