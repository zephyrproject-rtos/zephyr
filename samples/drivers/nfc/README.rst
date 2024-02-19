.. zephyr:code-sample:: nfc
   :name: NFC driver
   :relevant-api: nfc

   Configure NFC driver as Tag2

Overview
********

This sample demonstrates how to setup and configure the NFC driver.
It creates 3 different ntag records.

Building and Running
********************

The sample can be built and executed on boards supporting NFC.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/nfc
   :board: mg100
   :goals: build
   :compact:

Dependencies*
*************

This depends on libraries from the nRF Connect SDK and nrfxlib.

.. code-block:: none

   - name: nrf-sdk
     url: https://github.com/nrfconnect/sdk-nrf.git
     path: nrf
     revision: v2.5.2
     import:
       name-whitelist:
         - nrfxlib

Sample Output
=============

.. code-block:: console

    *** Booting nRF Connect SDK zephyr-v3.5.0 ***
    Starting NFC Text Record example
    Device = ok: NFC_DEV
    NFC configuration done (0)
    NFC-CALLBACK EVENT: 1
    NFC-CALLBACK EVENT: 3
    NFC-CALLBACK EVENT: 2
