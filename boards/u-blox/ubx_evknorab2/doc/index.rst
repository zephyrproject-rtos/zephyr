.. zephyr:board:: ubx_evknorab2

Overview
********

The NORA-B2 module is built on the nRF54L15, nRF54L10 and nRF54L05 SoCs
from Nordic Semiconductor, in different variants.

Hardware
********

EVK NORA-B2 has two crystal oscillators:

* High-frequency 32 MHz crystal oscillator (HFXO, module internal). Please
  refer to NORA-B2 SIM for information about internal capacitor settings.
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

Applications for the ``ubx_evknorab2/nrf54l15/cpuapp`` board can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Applications for the ``ubx_evknorab2/nrf54l15/cpuflpr`` board target need
to be built using sysbuild to include the ``vpr_launcher`` image for the
application core.

Enter the following command to compile ``hello_world`` for the FLPR core::
 west build -p -b ubx_evknorab2/nrf54l15/cpuflpr --sysbuild

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

To build and program the sample to the EVK NORA-B2, complete the following steps:

First, connect the EVK to you computer using the USB port on the EVK.
Next, build the sample by running the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ubx_evknorab2/nrf54l15/cpuapp
   :goals: build flash

Testing the LEDs and buttons in the EVK-NORA-B2
***********************************************

Test the EVK NORA-B2 with a :zephyr:code-sample:`blinky` sample.

.. _ubx_evknorab2_nrf54l05:

nRF54L05 emulation on EVK NORA-B2
*********************************

The ``ubx_evknorab2/nrf54l05`` board is a modified version of the EVK-NORA-B2
that enforces the limitations imposed by the nRF54L05 IC, which is a
cost-reduced variant of the original nRF54L15. This chipset is included in the NORA-B22 modules.
Since u-blox does not offer a development kit for the nRF54L05, you can use this board to develop for the
nRF54L05 IC while using the NORA-B2 Development Kit.

See `NORA-B2 product page`_ for more information.

.. _ubx_evknorab2_nrf54l10:

nRF54L10 emulation on EVK NORA-B2
*********************************

The ``ubx_evknorab2/nrf54l10`` board is a modified version of the EVK-NORA-B2
that enforces the limitations imposed by the nRF54L10 IC, which is a
cost-reduced variant of the original nRF54L15. This chipset is included in the NORA-B21 modules.
Since u-blox does not offer a development kit for the nRF54L10 you can use this board to develop for the
nRF54L10 while using the NORA-B2 Development Kit.

See `NORA-B2 product page`_ for more information.

References
**********

.. target-notes::

.. _NORA-B2 product page: https://www.u-blox.com/en/product/nora-b2-series
