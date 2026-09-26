.. zephyr:board:: ophelia5dk

Overview
********

.. note::
   You can find more information about the Ophelia-V radio module on the `Ophelia5 website`_.
   For the nRF54LM20A technical documentation and other resources (such as
   SoC Datasheet), see the `nRF54L documentation`_ page.

The Ophelia-V Development Kit hardware provides support for the Wuerth Electronic Ophelia-V
radio module, which is based on the Nordic Semiconductor nRF54LM20A Arm Cortex-M33 CPU.

Hardware
********

OPHELIA5 DK has two crystal oscillators:

* High-frequency 32 MHz crystal oscillator (HFXO)
* Low-frequency 32.768 kHz crystal oscillator (LFXO)

The LFXO uses the internal RC source by default. Nevertheless an external LFXO can be mounted
on the provided pins of the radio module.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``ophelia5dk/nrf54lm20a/cpuapp`` board target can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Applications for the ``ophelia5dk/nrf54lm20a/cpuflpr`` board target need
to be built using sysbuild to include the ``vpr_launcher`` image for the application core.

Enter the following command to compile ``hello_world`` for the FLPR core:

.. code-block:: console

   west build -p -b ophelia5dk/nrf54lm20a/cpuflpr samples/hello_world --sysbuild


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

To build and program the sample to the OPHELIA5 DK, complete the following steps:

First, connect the OPHELIA5 DK to you computer using the IMCU USB port on the DK.
Next, build the sample by running the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ophelia5dk/nrf54lm20a/cpuapp
   :goals: build flash

Testing the LEDs and buttons in the OPHELIA5 DK
************************************************

Test the OPHELIA5 DK with a :zephyr:code-sample:`blinky` sample.


.. _ophelia5dk_nrf54lm20a:

References
**********

.. target-notes::

.. _Ophelia5 website: https://www.we-online.de/katalog/de/article/262011022000
.. _nRF54L documentation: https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/app_dev/device_guides/nrf54l/index.html
.. _nRF54LM20A website: https://www.nordicsemi.com/Products/nRF54LM20A
