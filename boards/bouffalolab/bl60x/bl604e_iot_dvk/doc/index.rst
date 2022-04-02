.. zephyr:board:: bl604e_iot_dvk

Overview
********

BL602/BL604 is a Wi-Fi+BLE chipset introduced by Bouffalo Lab, which is used
for low power consumption and high performance application development.  The
wireless subsystem includes 2.4G radio, Wi-Fi 802.11b/g/n and BLE 5.0
baseband/MAC design.  The microcontroller subsystem includes a 32-bit RISC CPU
with low power consumption, cache and memory.  The power management unit
controls the low power consumption mode.  In addition, it also supports
various security features.  The external interfaces include SDIO, SPI, UART,
I2C, IR remote, PWM, ADC, DAC, PIR and GPIO.

The BL602 Development Board features a SiFive E24 32 bit RISC-V CPU with FPU,
it supports High Frequency clock up to 192Mhz, have 128k ROM, 276kB RAM,
2.4 GHz WIFI 1T1R mode, support 20 MHz, data rate up to 72.2 Mbps, BLE 5.0
with 2MB phy.  It is a secure MCU which supports Secure boot, ECC-256 signed
image, QSPI/SPI Flash On-The-Fly AES Decryption and PKA (Public Key
Accelerator).

Hardware
********

For more information about the Bouffalo Lab BL-60x MCU:

- `Bouffalo Lab BL60x MCU Website`_
- `Bouffalo Lab BL60x MCU Datasheet`_
- `Bouffalo Lab Development Zone`_
- `The RISC-V BL602 Book`_

Supported Features
==================

.. zephyr:board-supported-hw::

The default configuration can be found in the Kconfig
:zephyr_file:`boards/bouffalolab/bl60x/bl604e_iot_dvk/bl604e_iot_dvk_defconfig`.

System Clock
============

The BL604E Development Board is configured to run at max speed (192MHz).

Serial Port
===========

The ``bl604e_iot_dvk`` board uses UART0 as default serial port.  It is connected
to USB Serial converter and port is used for both program and console.


Programming and Debugging
*************************

Samples
=======

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample
application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: bl604e_iot_dvk
      :goals: build flash

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

   Then, press and release RST button

   .. code-block:: console

      *** Booting Zephyr OS build v4.1.0 ***
      Hello World! bl604e_iot_dvk/bl604e20q2i

Congratulations, you have ``bl604e_iot_dvk`` configured and running Zephyr.


.. _Bouffalo Lab BL60x MCU Website:
	https://en.bouffalolab.com/product/?type=detail&id=6

.. _Bouffalo Lab BL60x MCU Datasheet:
	https://github.com/bouffalolab/bl_docs/tree/main/BL602_DS/en

.. _Bouffalo Lab Development Zone:
	https://dev.bouffalolab.com/home?id=guest

.. _The RISC-V BL602 Book:
	https://lupyuen.github.io/articles/book

.. _Flashing Firmware to BL602:
	https://lupyuen.github.io/articles/book#flashing-firmware-to-bl602
