.. _rwcheck:

Disk Access Sample Application
##############################

Overview
********

This sample app demonstrates use of the disk access API,
it can be also used to test new disk access API drivers.

WARNING! First 100 sectors of the card will be erased!

Requirements
************

This project requires a SDHC or microSD card.
See the :ref:`sdhc_api` documentation for Zephyr implementation details.

Building and Running
********************

This sample can be built for a variety of boards.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/disk/speedcheck
   :board: stm32h747-disco
   :goals: build
   :compact:

To run this sample, a microSD card should be present in the
microSD slot. WARNING! First 100 sectors of the card will be erased!
