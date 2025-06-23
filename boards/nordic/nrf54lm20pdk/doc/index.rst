.. zephyr:board:: nrf54lm20pdk

Overview
********

.. note::

   All software for the nRF54LM20 SoC is experimental and hardware availability
   is restricted to the participants in the limited sampling program.

The nRF54LM20 Preview Development Kit hardware provides
support for the Nordic Semiconductor nRF54LM20 Arm Cortex-M33 CPU and
the following devices:

* CLOCK
* RRAM
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`GRTC (Global real-time counter)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`UARTE (Universal asynchronous receiver-transmitter)`

Hardware
********

nRF54LM20 PDK has two crystal oscillators:

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

Applications for the ``nrf54lm20pdk/nrf54lm20a/cpuapp`` board target can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Applications for the ``nrf54lm20pdk/nrf54lm20a/cpuflpr`` board target need
to be built using sysbuild to include the ``vpr_launcher`` image for the application core.

Enter the following command to compile ``hello_world`` for the FLPR core::
 west build -p -b nrf54lm20pdk/nrf54lm20a/cpuflpr --sysbuild

Flashing
========

As an example, this section shows how to build and flash the :zephyr:code-sample:`hello_world`
application.

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`.

To build and program the sample to the nRF54LM20 PDK, complete the following steps:

First, connect the nRF54LM20 PDK to you computer using the IMCU USB port on the PDK.
Next, build the sample by running the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nrf54lm20pdk/nrf54lm20a/cpuapp
   :goals: build flash

Testing the LEDs and buttons in the nRF54LM20 PDK
*************************************************

Test the nRF54LM20 PDK with a :zephyr:code-sample:`blinky` sample.
