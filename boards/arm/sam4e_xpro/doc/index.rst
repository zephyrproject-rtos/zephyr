.. _sam4e_xpro:

SAM4E Xplained Pro
###################

Overview
********

The SAM4E Xplained Pro evaluation kit is a development platform to evaluate the
Atmel SAM4E series microcontrollers.

.. image:: img/sam4e_xpro.jpg
     :width: 500px
     :align: center
     :alt: SAM4E Xplained Pro

Hardware
********

- ATSAM4E16E ARM Cortex-M4F Processor
- 12 MHz crystal oscillator
- internal 32.768 kHz crystal oscillator
- 2 x IS61WV5128BLL 4Mb SRAM
- MT29F2G08ABAEAWP 2Gb NAND
- SD card connector
- CAN-bus (TLE7250GVIOXUMA1 CAN Transceiver)
- Ethernet port (KSZ8081MNXIA phy)
- Micro-AB USB device
- Micro-AB USB debug interface supporting CMSIS-DAP, Virtual COM Port and Data
  Gateway Interface (DGI)
- One reset and one user pushbutton
- 1 yellow user LEDs


Supported Features
==================

The sam4e_xpro board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+
| USART     | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| ETHERNET  | on-chip    | ethernet                            |
+-----------+------------+-------------------------------------+
| WATCHDOG  | on-chip    | watchdog                            |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+

Other hardware features are not currently supported by Zephyr.

The default configuration can be found in the Kconfig
:zephyr_file:`boards/arm/sam4e_xpro/sam4e_xpro_defconfig`.

Connections and IOs
===================

The `SAM4E Xplained Pro User Guide`_ has detailed information about board
connections. Download the `SAM4E Xplained Pro documentation`_ for more detail.

System Clock
============

The SAM4E MCU is configured to use the 12 MHz internal oscillator on the board
with the on-chip PLL to generate an 120 MHz system clock.

Serial Port
===========

The ATSAM4E16E MCU has 2 UARTs and 2 USARTs. One of the UARTs (UART0) is
configured for the console and is available as a Virtual COM Port by EDBG USB
chip.

Programming and Debugging
*************************

Flashing the Zephyr project onto SAM4E MCU requires the `OpenOCD tool`_.
By default a factory new SAM4E chip will boot SAM-BA boot loader located in
the ROM, not the flashed image. This is determined by the value of GPNVM1
(General-Purpose NVM bit 1). The flash procedure will ensure that GPNVM1 is
set to 1 changing the default behavior to boot from Flash.

If your chip has a security bit GPNVM0 set you will be unable to program flash
memory or connect to it via a debug interface. The only way to clear GPNVM0
is to perform a chip erase procedure that will erase all GPNVM bits and the full
contents of the SAM4E flash memory:

- With the board power off, set a jumper on the J304 header.
- Turn the board power on. The jumper can be removed soon after the power is on
  (flash erasing procedure is started when the erase line is asserted for at
  least 230ms)

Flashing
========

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

#. Connect the SAM4E Xplained Pro board to your host computer using the
   USB debug port. Then build and flash the :ref:`hello_world`
   application.

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: sam4e_xpro
      :goals: build flash

   You should see "Hello World! arm" in your terminal.

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: sam4e_xpro
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _SAM4E Xplained Pro User Guide:
    http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-42216-SAM4E-Xplained-Pro_User-Guide.pdf

.. _SAM4E Xplained Pro documentation:
    http://ww1.microchip.com/downloads/en/DeviceDoc/SAM4E-Xplained-Pro_Design-Documentation.zip

.. _OpenOCD tool:
    http://openocd.org/

.. _SAM-BA:
    https://www.microchip.com/developmenttools/ProductDetails/PartNO/SAM-BA%20In-system%20Programmer
