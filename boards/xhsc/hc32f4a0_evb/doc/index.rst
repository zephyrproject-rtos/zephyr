.. zephyr:board:: hc32f4a0_evb

Overview
********

HC32F4A0_EVB,  also named as EV_F4A0_LQ176, is the official development board of XHSC, equipped with HC32F4A0SITB chip, based on ARM Cortex-M4 core, with a maximum frequency of 240 MHz, and rich on-board resources, which can give full play to the performance of HC32F4A0SITB chip.

Hardware
********

- MCU: HC32F4A0SITB, 240MHz, 2048KB FLASH, 512KB RAM
- External RAM:
  * SRAM: 1MB , IS62WV51216
  * SDRAM: 8MB, IS42S16400J
- External FLASH:
  * Nand: 256MB, MT29F2G08AB,
  * SPI NOR: 64MB , W25Q64
- Common Peripherals
  - LEDs: 3, user LEDs (LED0, LED1, LED2).
  - Buttons: 11, Matrix Keyboard (K1~K9), WAKEUP (K10), RESET (K11)
- Common Interfaces
  * USB to serial port
  * SD card interface
  * Ethernet interface
  * LCD interface
  * USB HS, USB FS, USB 3300
  * DVP interface
  * 3.5mm headphone jack,
  * Line in connector
  * speaker connector
- Debugging Interface
  * On-board DAP debugger
  * standard JTAG/SWD

More information about the board and MCU can be found at [XHSC](https://www.xhsc.com.cn/).

Supported Features
==================

The board configuration supports the following hardware features:

.. list-table::
   :header-rows: 1

   * - Peripheral
     - Kconfig option
     - Devicetree compatible
   * - GPIO
     - :kconfig:option:`CONFIG_GPIO`
     - :dtcompatible:`xhsc,hc32-gpio`
   * - NVIC
     - N/A
     - :dtcompatible:`arm,v7m-nvic`
   * - PINMUX
     - :kconfig:option:`CONFIG_PINCTRL`
     - :dtcompatible:`xhsc,hc32-pinctrl`
  * - USART
     * - :kconfig:option:`CONFIG_SERIAL`
     - :dtcompatible:`xhsc,hc32-uart`

Other hardware features are not yet supported on this Zephyr port.

Default Zephyr Peripheral Mapping
----------------------------------

- USART_1 TX/RX : PH15/PH13 (Virtual Port Com)
- LED11         : PC9
- K10/WKUP      : PA0

System Clock
------------

HC32F4A0_EVB System Clock could be driven by an internal or external
oscillator, as well as the PLL clock. By default, the System clock is
driven by the PLL clock at 240MHz, driven by an 8MHz high-speed external clock.

Serial Port
-----------

HC32F4A0_EVB board has 10 USARTs. The Zephyr console output is
assigned to UART1. Default settings are 115200 8N1.

Programming and Debugging
*************************

HC32F4A0_EVB board includes an XH-Link embedded debug tool interface.

Using XH-Link
=============

The HC32F4A0_EVB includes an onboard programmer/debugger (XH-Link) which
allow flash programming and debug over USB.

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: hc32f4a0_evb
      :goals: build
      :compact:

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyUSB0`. For example:

   .. code-block:: console

      $ minicom -D /dev/ttyUSB0 -o

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

#. To flash an image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: hc32f4a0_evb
      :goals: flash
      :compact:

   You should see "Hello World! hc32f4a0_evb/hc32f4a0" in your terminal.

#. To debug an image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: hc32f4a0_evb
      :goals: debug
      :compact:

.. _HC32F4A0 Datasheet, Reference Manual, HC32F4A0_EVB Schematics:
	https://www.xhsc.com.cn/product/112.html
