.. zephyr:board:: nrf7120dk

Overview
********

The nRF7120 Development Kit hardware provides support for the Nordic Semiconductor
nRF7120 Arm Cortex-M33 CPU.

Hardware
********

nRF7120 DK has two crystal oscillators:

* High-frequency 64 MHz crystal oscillator (HFXO)
* Low-frequency 32.768 kHz crystal oscillator (LFXO)

The crystal oscillators can be configured to use either
internal or external capacitors.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``nrf7120dk/nrf7120/cpuapp`` board target can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Applications for the ``nrf7120dk/nrf7120/cpuflpr`` board target need
to be built using sysbuild to include the ``vpr_launcher`` image for the application core.

Enter the following command to compile ``hello_world`` for the FLPR core:

.. code-block:: console

   west build -p -b nrf7120dk/nrf7120/cpuflpr --sysbuild


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

To build and program the sample to the nRF7120 DK, complete the following steps:

First, connect the nRF7120 DK to you computer using the IMCU USB port on the DK.
Next, build the sample by running the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nrf7120dk/nrf7120/cpuapp
   :goals: build flash

Testing the LEDs and buttons in the nRF7120 DK
************************************************

Test the nRF7120 DK with a :zephyr:code-sample:`blinky` sample.
