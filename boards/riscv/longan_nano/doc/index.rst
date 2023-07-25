.. _longan_nano:

Sipeed Longan Nano
##################

.. image:: img/longan_nano.jpg
     :align: center
     :alt: longan_nano

Overview
********

The Sipeed Longan Nano and Longan Nano Lite is an simple and tiny development board with
an GigaDevice GD32VF103 SoC that based on N200 RISC-V IP core by Nuclei system technology.
More information can be found on:

- `Sipeed Longan website <https://longan.sipeed.com/en/>`_
- `GD32VF103 datasheet <https://www.gigadevice.com/datasheet/gd32vf103xxxx-datasheet/>`_
- `GD32VF103 user manual <https://www.gd32mcu.com/data/documents/userManual/GD32VF103_User_Manual_Rev1.4.pdf>`_
- `Nuclei website <https://www.nucleisys.com/download.php>`_
- `Nuclei Bumblebee core documents <https://github.com/nucleisys/Bumblebee_Core_Doc>`_
- `Nuclei ISA Spec <https://doc.nucleisys.com/nuclei_spec/>`_

Hardware
********

- 4 x universal 16-bit timer
- 2 x basic 16-bit timer
- 1 x advanced 16-bit timer
- Watchdog timer
- RTC
- Systick
- 3 x USART
- 2 x I2C
- 3 x SPI
- 2 x I2S
- 2 x CAN
- 1 x USBFS(OTG)
- 2 x ADC(10 channel)
- 2 x DAC

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
     - :dtcompatible:`gd,gd32-gpio`
   * - Machine timer
     - :kconfig:option:`CONFIG_RISCV_MACHINE_TIMER`
     - :dtcompatible:`riscv,machine-timer`
   * - Nuclei ECLIC Interrupt Controller
     - :kconfig:option:`CONFIG_NUCLEI_ECLIC`
     - :dtcompatible:`nuclei,eclic`
   * - PWM
     - :kconfig:option:`CONFIG_PWM`
     - :dtcompatible:`gd,gd32-pwm`
   * - USART
     - :kconfig:option:`CONFIG_SERIAL`
     - :dtcompatible:`gd,gd32-usart`
   * - I2C
     - :kconfig:option:`CONFIG_I2C`
     - :dtcompatible:`gd,gd32-i2c`
   * - DAC
     - :kconfig:option:`CONFIG_DAC`
     - :dtcompatible:`gd,gd32-dac`

Serial Port
===========

USART0 is on the opposite end of the USB.
Connect to TX0 (PA9) and RX0 (PA10).

Programming and debugging
*************************

Building & Flashing
===================

Here is an example for building the :ref:`blinky-sample` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: longan_nano
   :goals: build flash

When using a custom toolchain it should be enough to have the downloaded
version of the binary in your ``PATH``.

The default runner tries to flash the board via an external programmer using openocd.
To flash via the USB port, select the DFU runner when flashing:

.. code-block:: console

   west flash --runner dfu-util

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:ref:`blinky-sample` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: longan_nano
   :maybe-skip-config:
   :goals: debug
