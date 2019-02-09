.. _nrf52840_pca10059:

nRF52840-PCA10059
#################

Overview
********

The nRF52840 PCA10059 (USB Dongle) hardware provides support for the Nordic
Semiconductor nRF52840 ARM Cortex-M4F CPU and the following devices:

* :abbr:`ADC (Analog to Digital Converter)`
* CLOCK
* FLASH
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`I2C (Inter-Integrated Circuit)`
* :abbr:`MPU (Memory Protection Unit)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`PWM (Pulse Width Modulation)`
* RADIO (Bluetooth Low Energy and 802.15.4)
* :abbr:`RTC (nRF RTC System Clock)`
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UART (Universal asynchronous receiver-transmitter)`
* :abbr:`USB (Universal Serial Bus)`
* :abbr:`WDT (Watchdog Timer)`

.. figure:: img/nrf52840_pca10059.jpg
     :width: 442px
     :align: center
     :alt: nRF52840 Dongle

     nRF52840 PCA10059

More information about the board can be found at the
`nRF52840 Dongle website`_. The `Nordic Semiconductor Documentation library`_
contains the processor's information and the datasheet.

Hardware
********

The ``nrf52840_pca10059`` has two external oscillators. The frequency of
the slow clock is 32.768 kHz. The frequency of the main clock
is 32 MHz.

Supported Features
==================

The ``nrf52840_pca10059`` board configuration supports the following
hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| ADC       | on-chip    | adc                  |
+-----------+------------+----------------------+
| CLOCK     | on-chip    | clock_control        |
+-----------+------------+----------------------+
| FLASH     | on-chip    | flash                |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| RADIO     | on-chip    | Bluetooth,           |
|           |            | ieee802154           |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| USB       | on-chip    | usb                  |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Other hardware features are not supported by the Zephyr kernel.
See `nRF52840 Dongle website`_ and `Nordic Semiconductor Documentation library`_
for a complete list of nRF52840 PCA10059 Development Kit board hardware features.

Connections and IOs
===================

LED
---

* LED0 (green) = P0.6
* LED1 (red)   = P0.8
* LED1 (green) = P1.9
* LED1 (blue)  = P0.12

Push buttons
------------

* BUTTON1 = SW1 = P1.6
* RESET   = SW2 = P0.18

Programming and Debugging
*************************

Applications for the ``nrf52840_pca10059`` board configuration can be
built in the usual way (see :ref:`build_an_application` for more details).
There are two ways to program the board, with the DFU tool or with an external
debugger chip.

Flashing with a device firmware update (DFU)
============================================

The board is factory-programmed with Nordic's bootloader from Nordic's nRF5 SDK.
This section covers the steps required to program the board with a Zephyr
application using Nordic's DFU tool nrfutil.

Follow the instructions on `nrfutil GitHub`_ to install the necessary software.

Compile a Zephyr application as usual,
here is an example for the :ref:`blinky-sample` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nrf52840_pca10059
   :goals: build

Create an application package, using nrfutil::

	nrfutil pkg generate --hw-version 52 --sd-req=0x00 \
		--application zephyr.hex --application-version 1 pkg.zip

Flash it onto the board::

	nrfutil dfu usb_serial -pkg pkg.zip -p /dev/ttyACM0

Observe the green LED on the board blinking.

For more information on these steps visit: `Nordic Semiconductor USB DFU`_ and
`nrfutil GitHub`_ pages.

Chainloading the MCUBoot bootloader
===================================

It is possible to use the nRF5 bootloader alongside MCUBoot. To do so,
program the board with MCUBoot as a Zephyr application, following
the steps above. Then, prepare to compile an application with MCUBoot support.

Select :option: `CONFIG_BOOTLOADER_MCUBOOT`, under "Boot options" and set
:option: `CONFIG_TEXT_SECTION_OFFSET` under "Build and Link features",
"Linker options" to 0x200 to ensure the code is offset to account for MCUboot
firmware image metadata.

Sign the resulting firmware image using the imgtool utility as follows::

	imgtool sign -S 0x5e000 --key mcuboot/root-rsa-2048.pem \
		--header-size 0x200 --align 8 --version 3.0 firmware.bin signed.bin

Enter MCUboot serial recovery mode by inserting the device into the USB port
while holding BUTTON1 down. Keep in mind that resetting the device using the
RESET button will always enter Nordic nRF5 bootloader's DFU mode.

Finally, perform a firmware update using mcumgr as follows::

	mcumgr --conntype=serial --connstring='dev=/dev/ttyACM0,baud=115200' \
		image upload -e signed.bin

and reset the device::

	mcumgr --conntype=serial --connstring='dev=/dev/ttyACM0,baud=115200' reset

For more information about these steps refer to: `MCUboot`_ and
`mcumgr`_.

Flashing with an external JLink programmer
===========================================

Flashing Zephyr onto the ``nrf52840_pca10059`` with an external J-Link
programmer requires an SWD header to be attached on the back side of the board.

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`.

Locate the DTS file for the board under: boards/arm/nrf52840_pca10059.
This file requires a small modification to use a different partition table.
Edit the include directive to include "fstab-debugger" instead of "fstab-stock".

Then build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :ref:`blinky-sample` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nrf52840_pca10059
   :goals: build flash

Observe the LED on the board blinking.

Debugging
=========

The ``nrf52840_pca10059`` board does not have an on-board J-Link debug IC
as some nRF5x development boards, however, instructions from the
:ref:`nordic_segger` page also apply to this board, with the additional step
of connecting an external debugger.

Testing the LEDs and buttons on the nRF52840 PCA10059
*****************************************************

There are 2 samples that allow you to test that the buttons (switches) and LEDs on
the board are working properly with Zephyr:

* :ref:`blinky-sample`

You can build and program the examples to make sure Zephyr is running correctly
on your board.


References
**********

.. target-notes::

.. _nRF52840 Dongle website: https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF52840-Dongle
.. _Nordic Semiconductor Documentation library: https://www.nordicsemi.com/DocLib
.. _J-Link Software and documentation pack: https://www.segger.com/jlink-software.html
.. _Nordic Semiconductor USB DFU: https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v15.2.0%2Fsdk_app_serial_dfu_bootloader.html
.. _nrfutil GitHub: https://github.com/NordicSemiconductor/pc-nrfutil
.. _MCUboot: https://github.com/runtimeco/mcuboot/blob/master/docs/readme-zephyr.md
.. _mcumgr: https://github.com/apache/mynewt-mcumgr-cli
