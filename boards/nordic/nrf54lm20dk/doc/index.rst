.. zephyr:board:: nrf54lm20dk

Overview
********

.. note::
   You can find more information about the nRF54LM20B SoC on the `nRF54LM20B website`_.
   For the nRF54LM20B technical documentation and other resources (such as
   SoC Datasheet), see the `nRF54L documentation`_ page.

The nRF54LM20 Development Kit hardware provides support for the Nordic Semiconductor
nRF54LM20B Arm Cortex-M33 CPU and the following devices:

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

nRF54LM20 DK has two crystal oscillators:

* High-frequency 32 MHz crystal oscillator (HFXO)
* Low-frequency 32.768 kHz crystal oscillator (LFXO)

The crystal oscillators can be configured to use either
internal or external capacitors.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``nrf54lm20dk/nrf54lm20b/cpuapp`` board target can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Applications for the ``nrf54lm20dk/nrf54lm20b/cpuflpr`` board target need
to be built using sysbuild to include the ``vpr_launcher`` image for the application core.

Enter the following command to compile ``hello_world`` for the FLPR core:

.. code-block:: console

   west build -p -b nrf54lm20dk/nrf54lm20b/cpuflpr --sysbuild


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

To build and program the sample to the nRF54LM20 DK, complete the following steps:

First, connect the nRF54LM20 DK to you computer using the IMCU USB port on the DK.
Next, build the sample by running the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nrf54lm20dk/nrf54lm20b/cpuapp
   :goals: build flash

Testing the LEDs and buttons in the nRF54LM20 DK
************************************************

Test the nRF54LM20 DK with a :zephyr:code-sample:`blinky` sample.


.. _nrf54lm20dk_nrf54lm20a:

nRF54LM20A emulation on nRF54LM20 DK
************************************

The ``nrf54lm20dk/nrf54lm20a`` board is a modified version of the :zephyr:board:`nrf54lm20dk`
that enforces the limitations imposed by the nRF54LM20A IC, which is the NPU-less variant
of the original nRF54LM20B IC. Since Nordic does not offer a development kit for the nRF54LM20A,
you can use this board to develop for this IC while using the nRF54LM20 Development Kit (PCA10184).

See `nRF54LM20A website`_ for the official reference on the IC itself.


References
**********

.. target-notes::

.. _nRF54LM20B website: https://www.nordicsemi.com/Products/nRF54LM20B
.. _nRF54L documentation: https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/app_dev/device_guides/nrf54l/index.html
.. _nRF54LM20A website: https://www.nordicsemi.com/Products/nRF54LM20A
