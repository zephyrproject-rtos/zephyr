.. zephyr:board:: bl54l15_dvk

Overview
********

.. note::
   You can find more information about the BL54L15 module on the `BL54L15 website`_.

   You can find more information about the BL54L10 module on the `BL54L10 website`_.

   You can find more information about the underlying nRF54L15 SoC on the
   `nRF54L15 website`_. For the nRF54L15 technical documentation and other
   resources (such as SoC Datasheet), see the `nRF54L15 documentation`_ page.

The BL54L15 Development Kit provides support for the Ezurio BL54L15 and BL54L10
range of modules.

The modules are based on the Nordic Semiconductor nRF54L15 and nRF54L10 Arm Cortex-M33 CPUs.

The BL54L15 module incorporates the QFN package nRF54L15 (1524kB Flash, 256kB RAM). The BL54L10
module the QFN package nRF54L10 (1022kB Flash, 192kB RAM). Both parts feature up to 31 configurable
GPIOs and BLE Radio TX Power up to 7dBm.

The modules include the following devices:

* :abbr:`SAADC (Successive Approximation Analog to Digital Converter)`
* CLOCK
* RRAM
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`TWIM (I2C-compatible two-wire interface master with EasyDMA)`
* MEMCONF
* :abbr:`MPU (Memory Protection Unit)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`PWM (Pulse Width Modulation)`
* :abbr:`GRTC (Global real-time counter)`
* Segger RTT (RTT Console)
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UARTE (Universal asynchronous receiver-transmitter)`
* :abbr:`WDT (Watchdog Timer)`

Hardware
********

The BL54L15 DVK has two crystal oscillators:

* High-frequency 32 MHz crystal oscillator (HFXO)
* Low-frequency 32.768 kHz crystal oscillator (LFXO)

The crystal oscillators can be configured to use either
internal or external capacitors.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

Applications for the ``bl54l15_dvk/nrf54l15/cpuapp`` and ``bl54l15_dvk/nrf54l10/cpuapp``
board targets can be built, flashed, and debugged in the usual way.
See :ref:`build_an_application` and :ref:`application_run` for more
details on building and running.

Applications for the ``bl54l15_dvk/nrf54l15/cpuflpr`` board target need to be built
using sysbuild to include the ``vpr_launcher`` image for the application core.

Enter the following command to compile ``hello_world`` for the FLPR core:

.. code-block:: console

   west build -p -b bl54l15_dvk/nrf54l15/cpuflpr --sysbuild

Flashing
========

As an example, this section shows how to build and flash the :zephyr:code-sample:`hello_world`
application.

.. warning::

   When programming the device, you might get an error similar to the following message::

    ERROR: The operation attempted is unavailable due to readback protection in
    ERROR: your device. Please use --recover to unlock the device.

   This error occurs when readback protection is enabled.
   To disable the readback protection, you must *recover* your device.

   Enter the following command to recover the core::

    west flash --recover

   The ``--recover`` command erases the flash memory and then writes a small binary into
   the recovered flash memory.
   This binary prevents the readback protection from enabling itself again after a pin
   reset or power cycle.

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`.

To build and program the sample to the BL54L15 DVK, complete the following steps:

First, connect the BL54L15 DVK to your computer using the IMCU USB port on the DVK.
Next, build the sample by running the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: bl54l15_dvk/nrf54l15/cpuapp
   :goals: build flash

Testing the LEDs and buttons on the BL54L15 DVK
************************************************

Test the BL54L15 DVK with a :zephyr:code-sample:`blinky` sample.

.. _BL54L15 website: https://www.ezurio.com/product/bl54l15-series-bluetooth-le-80215-4-nfc
.. _BL54L10 website: https://www.ezurio.com/wireless-modules/bluetooth-modules/bl54-series/bl54l10-series-bluetooth-le-802-15-4-nfc
.. _nRF54L15 website: https://www.nordicsemi.com/Products/nRF54L15
.. _nRF54L15 documentation: https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/app_dev/device_guides/nrf54l/index.html

.. _bl54l15_dvk_nrf54l10:

BL54L10 emulation on BL54L15 DVK
********************************

Overview
========

The ``bl54l15_dvk/nrf54l10`` board is a modified version of the BL54L15 DVK
that enforces the limitations imposed by the nRF54L10 IC, upon which the BL54L10 is based.
The nRF54L10 is a cost-reduced variant of the original nRF54L15. Since Ezurio does not offer a
development kit for the BL54L10 you can use this board to develop for this
module while using the BL54L15 Development Kit (453-00001-K1, 453-00044-K1).

See above for more information about the development board. Refer to
the `BL54L10 website`_ for details of the BL54L10 module and the `nRF54L10 website`_ for
the official reference on the underlying SoC.

.. _BL54L10 website: https://www.ezurio.com/wireless-modules/bluetooth-modules/bl54-series/bl54l10-series-bluetooth-le-802-15-4-nfc
.. _nRF54L10 website: https://www.nordicsemi.com/Products/nRF54L10
