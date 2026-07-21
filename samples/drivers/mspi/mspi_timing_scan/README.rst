.. zephyr:code-sample:: mspi-timing-scan
   :name: Ambiq MSPI timing scan
   :relevant-api: flash_interface

   Find the appropriate timing for a given device on a given board.

Overview
********

This sample demonstrates the usage of ambiq timing scan utility.

Building and Running
********************

The application will build only for a target that has a :ref:`devicetree <dt-guide>`
``flash0`` or ``psram0`` alias depending on the interface used.
They refers to an entry with the following bindings as a compatible:

* :dtcompatible:`ambiq,mspi-device`

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/mspi/mspi_timing_scan
   :board: apollo510_evb
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.4.0-27775-g750ed00d564b ***
   Starting MSPI Timing Scan.
   <inf> mspi_ambiq_timing_scan: TxNeg=0, RxNeg=0, RxCap=0, Turnaround=5
   <inf> mspi_ambiq_timing_scan:     TxDQSDelay: 0, RxDQSDelay Scan = 0x0007FFFE, Window size = 18
   <inf> mspi_ambiq_timing_scan:     TxDQSDelay: 1, RxDQSDelay Scan = 0x0007FFFF, Window size = 19
   <inf> mspi_ambiq_timing_scan:     TxDQSDelay: 1, RxDQSDelay Scan = 0x0007FFFF, Window size = 19
   <inf> mspi_ambiq_timing_scan:     TxDQSDelay: 2, RxDQSDelay Scan = 0x0007FFFE, Window size = 18
   <inf> mspi_ambiq_timing_scan:     TxDQSDelay: 3, RxDQSDelay Scan = 0x0007FFFF, Window size = 19
   <inf> mspi_ambiq_timing_scan:     TxDQSDelay: 4, RxDQSDelay Scan = 0x0007FFFE, Window size = 18
   <inf> mspi_ambiq_timing_scan:     TxDQSDelay: 5, RxDQSDelay Scan = 0x0005FD54, Window size = 7
   <inf> mspi_ambiq_timing_scan:     TxDQSDelay: 6, RxDQSDelay Scan = 0x00000000, Window size = 0
   <inf> mspi_ambiq_timing_scan:     TxDQSDelay: 7, RxDQSDelay Scan = 0x00000000, Window size = 0
   <inf> mspi_ambiq_timing_scan:     TxDQSDelay: 8, RxDQSDelay Scan = 0x00000000, Window size = 0
   <inf> mspi_ambiq_timing_scan:     TxDQSDelay: 9, RxDQSDelay Scan = 0x00000000, Window size = 0
   <inf> mspi_ambiq_timing_scan:     TxDQSDelay: 10, RxDQSDelay Scan = 0x00000000, Window size = 0
   <inf> mspi_ambiq_timing_scan: Selected setting: TxNeg=0, RxNeg=0, RxCap=0, Turnaround=5,TxDQSDelay=2, RxDQSDelay=9
   MSPI Timing Scan is successful.
