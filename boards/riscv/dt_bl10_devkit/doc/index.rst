.. _dt_bl10_devkit:

BL602 Development Board
#######################

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

.. image:: img/dt_bl10_devkit.jpg
     :width: 450px
     :align: center
     :alt: dt_bl10_devkit

Hardware
********

For more information about the Bouffalo Lab BL-602 MCU:

- `Bouffalo Lab BL602 MCU Website`_
- `Bouffalo Lab BL602 MCU Datasheet`_
- `Bouffalo Lab Development Zone`_
- `dt_bl10_devkit Schematic`_
- `Doctors of Intelligence & Technology (www.doiting.com)`_
- `The RISC-V BL602 Book`_

Supported Features
==================

The board configuration supports the following hardware features:

+-----------+------------+-----------------------+
| Interface | Controller | Driver/Component      |
+===========+============+=======================+
| MTIMER    | on-chip    | RISC-V Machine Timer  |
+-----------+------------+-----------------------+
| PINCTRL   | on-chip    | pin muxing            |
+-----------+------------+-----------------------+
| UART      | on-chip    | serial port-polling   |
+-----------+------------+-----------------------+


The default configurations can be found in the Kconfig
:zephyr_file:`boards/risc/dt_bl10_devkit/dt_bl10_devkit_defconfig`.

System Clock
============

The BL602 Development Board is configured to run at max speed (192MHz).

Serial Port
===========

The dt_bl10_devkit_ uses UART0 as default serial port.  It is connected to
USB Serial converter and port is used for both program and console.


Programming and Debugging
*************************

BL Flash tool
=============

The BL-602 have a ROM bootloader that allows user flash device by serial port.
There are some tools available at internet and this will describe one of them.
The below guide was created based on RISC-V BL602 Book, chapter 3
`Flashing Firmware to BL602`_.

#. `Install Rustup`_

#. Clone blflash rust version

   .. code-block:: console

      $ git clone --recursive https://github.com/spacemeowx2/blflash

#. Build blflash

   .. code-block:: console

      $ cd blflash
      $ cargo build --release

#. Install blflash. The recommended use is copy to home folder

   .. code-block:: console

      $ cp blflash ~/bin/

#. Test

   .. code-block:: console

      $ blflash -V

   It will print blflash version

   .. code-block:: console

      $ blflash 0.3.3

Samples
=======

#. Build the Zephyr kernel and the :ref:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: dt_bl10_devkit
      :goals: build
      :compact:

#. To flash an image using blflash runner:

   #. Press D8 button

   #. Press and release EN button

   #. Release D8 button

   .. code-block:: console

      west flash -r blflash

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

   Then, press and release EN button

   .. code-block:: console

      *** Booting Zephyr OS build zephyr-v2.6.0-1729-g22140c728537  ***
      Hello World! dt_bl10_devkit

Congratulations, you have `dt_bl10_devkit`_ configured and running Zephyr.


.. _Bouffalo Lab BL602 MCU Website:
	https://www.bouffalolab.com/bl602

.. _Bouffalo Lab BL602 MCU Datasheet:
	https://github.com/bouffalolab/bl_docs/tree/main/BL602_DS/en

.. _Bouffalo Lab Development Zone:
	https://dev.bouffalolab.com/home?id=guest

.. _dt_bl10_devkit Schematic:
	https://github.com/SmartArduino/Doiting_BL/blob/master/board/DT-BL10%20User%20Mannual.pdf

.. _Doctors of Intelligence & Technology (www.doiting.com):
	https://www.doiting.com

.. _Install Rustup:
	https://rustup.rs/

.. _The RISC-V BL602 Book:
	https://lupyuen.github.io/articles/book

.. _Flashing Firmware to BL602:
	https://lupyuen.github.io/articles/book#flashing-firmware-to-bl602
