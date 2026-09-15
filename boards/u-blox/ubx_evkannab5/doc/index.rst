.. zephyr:board:: ubx_evkannab5

Overview
********

The ANNA-B50 module is built on the nRF54L15 SoC from Nordic
Semiconductor.

Hardware
********

EVK ANNA-B50 has two crystal oscillators:

* High-frequency 32 MHz crystal oscillator (HFXO, module internal). Please
  refer to ANNA-B50 SIM for information about internal capacitor settings.
* Low-frequency 32.768 kHz crystal oscillator (LFXO)

The LFXO crystal oscillator can be configured to use either
internal or external capacitors. By default, the internal capacitors are used.
For more information about configuring the oscillators, refer to the
:nrf_clock_control: documentation.

Supported features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``ubx_evkannab5/nrf54l15/cpuapp`` board can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Applications for the ``ubx_evkannab5/nrf54l15/cpuflpr`` board target need
to be built using sysbuild to include the ``vpr_launcher`` image for the
application core.

Enter the following command to compile ``hello_world`` for the FLPR core::
 west build -p -b ubx_evkannab5/nrf54l15/cpuflpr samples/hello_world --sysbuild

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

To build and program the sample to the EVK ANNA-B5, complete the following steps:

First, connect the EVK to you computer using the USB port on the EVK.
Next, build the sample by running the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ubx_evkannab5/nrf54l15/cpuapp
   :goals: build flash

Testing the LEDs and buttons in the EVK-ANNA-B5
***********************************************

Test the EVK-ANNA-B5with a :zephyr:code-sample:`blinky` sample.
