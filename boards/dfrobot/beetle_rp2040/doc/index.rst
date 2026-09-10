.. zephyr:board:: beetle_rp2040

Overview
********

The `DFRobot Beetle RP2040`_ board is based on the RP2040
microcontroller from Raspberry Pi Ltd. The board has
a USB type C connector, and uses a large pad design.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 28 Mbyte QSPI flash
- 6 GPIO pins
- 2 ADC pins
- I2C
- SPI
- UART
- USB type C connector
- Reset and boot buttons
- Blue LED


Default Zephyr Peripheral Mapping
=================================

.. rst-class:: rst-columns

    - GP0 : GPIO0
    - GP1 : GPIO1
    - GP2 : GPIO2
    - GP3 : GPIO3
    - GP4 UART1 TX : GPIO4
    - GP5 UART1 RX : GPIO5
    - GP28 ADC channel 2 : GPIO28
    - GP29 ADC channel 3 : GPIO29
    - Blue LED : GPIO13


See also `pinout`_.


Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The DFRobot Beetle RP2040 board does not expose the SWDIO and SWCLK pins, so programming
must be done via the USB port. Press and hold the BOOT button, and then press the RST button,
and the device will appear as a USB mass storage unit.
Building your application will result in a :file:`build/zephyr/zephyr.uf2` file.
Drag and drop the file to the USB mass storage unit, and the board will be reprogrammed.

For more details on programming RP2040-based boards, see
:ref:`rpi_pico_programming_and_debugging`.


Flashing
========

To run the :zephyr:code-sample:`blinky` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky/
   :board: beetle_rp2040
   :goals: build flash

Try also the :zephyr:code-sample:`hello_world`, :zephyr:code-sample:`usb-cdc-acm-console` and
:zephyr:code-sample:`adc_dt` samples.


References
**********

.. target-notes::

.. _DFRobot Beetle RP2040:
    https://www.dfrobot.com/product-2615.html

.. _pinout:
    https://wiki.dfrobot.com/SKU_DFR0959_Beetle_RP2040
