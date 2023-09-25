.. _comparator-sample:

Comparator
##########

Overview
********

This sample demonstrates how to use the Comparator Driver API.

Requirements
************

The comparator device to be used by the sample must be provided with
an initial configuration in devicetree and its node must be marked
with the ``comp_dev`` label.
See :zephyr_file:`boards/nrf52840dk_nrf52840.overlay
<samples/drivers/comparator/boards/nrf52840dk_nrf52840.overlay>` for an example
of such setup.

Building and Running
********************

The code can be found in :zephyr_file:`samples/drivers/comparator`.

To build and flash this sample for the :ref:`nrf52840dk_nrf52840`:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/comparator
   :board: nrf52840dk_nrf52840
   :goals: build flash
   :compact:
