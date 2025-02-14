.. zephyr:board:: adafruit_macropad_rp2040

Adafruit MacroPad RP2040
########################

Overview
********

The Adafruit MacroPad RP2040 is a 3x4 mechanical keyboard development board featuring
the Raspberry Pi RP2040 microcontroller. It includes 12 mechanical key switches with
individual RGB NeoPixels, a rotary encoder with push button, a 128x64 OLED display,
and a small speaker for audio feedback.

Hardware
********

- Dual core Cortex-M0+ at up to 133MHz
- 264KB of SRAM
- 8MB of QSPI flash memory
- 12 mechanical key switches (Cherry MX compatible)
- 12 RGB NeoPixel LEDs (one per key)
- 128x64 monochrome OLED display (SH1106)
- Rotary encoder with push button
- Small speaker with Class D amplifier
- STEMMA QT / Qwiic I2C connector
- USB Type-C connector
- Reset button
- 4x M3 mounting holes

Supported Features
******************

The ``adafruit_macropad_rp2040`` board target supports the following hardware features:

.. list-table::
   :header-rows: 1

   * - Peripheral
     - Kconfig option
     - Devicetree compatible
   * - NVIC
     - N/A
     - :dtcompatible:`arm,v6m-nvic`
   * - UART
     - :kconfig:option:`CONFIG_SERIAL`
     - :dtcompatible:`raspberrypi,pico-uart`
   * - GPIO
     - :kconfig:option:`CONFIG_GPIO`
     - :dtcompatible:`raspberrypi,pico-gpio`
   * - ADC
     - :kconfig:option:`CONFIG_ADC`
     - :dtcompatible:`raspberrypi,pico-adc`
   * - I2C
     - :kconfig:option:`CONFIG_I2C`
     - :dtcompatible:`snps,designware-i2c`
   * - SPI
     - :kconfig:option:`CONFIG_SPI`
     - :dtcompatible:`raspberrypi,pico-spi`
   * - USB Device
     - :kconfig:option:`CONFIG_USB_DEVICE_STACK`
     - :dtcompatible:`raspberrypi,pico-usbd`
   * - HWINFO
     - :kconfig:option:`CONFIG_HWINFO`
     - N/A
   * - Watchdog Timer (WDT)
     - :kconfig:option:`CONFIG_WATCHDOG`
     - :dtcompatible:`raspberrypi,pico-watchdog`
   * - PWM
     - :kconfig:option:`CONFIG_PWM`
     - :dtcompatible:`raspberrypi,pico-pwm`
   * - Flash
     - :kconfig:option:`CONFIG_FLASH`
     - :dtcompatible:`raspberrypi,pico-flash-controller`
   * - UART (PIO)
     - :kconfig:option:`CONFIG_SERIAL`
     - :dtcompatible:`raspberrypi,pico-uart-pio`
   * - Display
     - :kconfig:option:`CONFIG_DISPLAY`
     - :dtcompatible:`sinowealth,sh1106`
   * - LED Strip (12 pixels)
     - :kconfig:option:`CONFIG_LED_STRIP`
     - :dtcompatible:`worldsemi,ws2812-rpi_pico-pio`
   * - Rotary Encoder
     - :kconfig:option:`CONFIG_INPUT`
     - :dtcompatible:`gpio-qdec`

Programming and Debugging
*************************

Applications for the ``adafruit_macropad_rp2040`` board target can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Building and Flashing
*********************

The MacroPad RP2040 has a built-in UF2 bootloader which can be entered by holding down the rotary
encoder button (BOOT) and, while continuing to hold it, pressing and releasing the reset button.
A "RPI-RP2" drive should appear on your host machine.

Here is an example for building and flashing the :zephyr:code-sample:`blinky` sample application
using UF2.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: adafruit_macropad_rp2040
   :goals: build flash
   :flash-args: --runner uf2

References
**********

.. target-notes::

- `Adafruit MacroPad RP2040 Product Page <https://www.adafruit.com/product/5128>`_
- `Adafruit MacroPad RP2040 Learn Guide <https://learn.adafruit.com/adafruit-macropad-rp2040>`_
