.. _zsai_sample:

ZSAI Usage Sample
#################

Overview
********

A simple sample that shows how the Zephyr Storage Access Interface can
be used to access devices requiring and not requiring erase.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/storage/zsai
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

To build for another board, change "qemu_x86" above to that board's name.

Sample Output
=============

.. code-block:: console

    *** Booting Zephyr OS build zephyr-v3.5.0-561-g1939dcf17441 ***
    Starting ZSAI sample on nrf52840dk_nrf52840
    Device uses write block size 4
    Device has size 1048576
    Device requires erase prior to write
    For offset 1048536, got page with offset 1044480 and size 4096
    Check if offset for write is erased
    Erase was ok
    Writing DATA to device
    Reading device back to buffer
    Data and buffer matches
