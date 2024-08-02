.. _nrf54l20pdk_nrf54l20:

nRF54L20 PDK
############

Overview
********

.. note::

   All software for the nRF54L20 SoC is experimental and hardware availability
   is restricted to the participants in the limited sampling program.

The nRF54L20 Preview Development Kit hardware provides
support for the Nordic Semiconductor nRF54L20 Arm Cortex-M33 CPU and
the following devices:

* CLOCK
* RRAM
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`GRTC (Global real-time counter)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`UARTE (Universal asynchronous receiver-transmitter)`

Hardware
********

nRF54L20 PDK has two crystal oscillators:

* High-frequency 32 MHz crystal oscillator (HFXO)
* Low-frequency 32.768 kHz crystal oscillator (LFXO)

The crystal oscillators can be configured to use either
internal or external capacitors.

Supported Features
==================

The ``nrf54l20pdk/nrf54l20/cpuapp`` board target configuration supports the following
hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| CLOCK     | on-chip    | clock_control        |
+-----------+------------+----------------------+
| RRAM      | on-chip    | flash                |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| GRTC      | on-chip    | system clock         |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| UARTE     | on-chip    | serial               |
+-----------+------------+----------------------+

Other hardware features have not been enabled yet for this board.

Programming and Debugging
*************************

Applications for the ``nrf54l20pdk/nrf54l20/cpuapp`` board target can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Flashing
========

As an example, this section shows how to build and flash the :ref:`hello_world`
application.

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`.

To build and program the sample to the nRF54L20 PDK, complete the following steps:

First, connect the nRF54L20 PDK to you computer using the IMCU USB port on the PDK.
Next, build the sample by running the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nrf54l20pdk/nrf54l20/cpuapp
   :goals: build flash

Testing the LEDs and buttons in the nRF54L20 PDK
************************************************

Test the nRF54L20 PDK with a :zephyr:code-sample:`blinky` sample.
