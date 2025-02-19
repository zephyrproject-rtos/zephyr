.. zephyr:code-sample:: mspi-timing-scan
   :name: Ambiq MSPI timing scan
   :relevant-api: flash_interface memc_interface

   Find the approriate timing for a given device on a given board.

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
   :board: apollo5_eb
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.4.0-27775-g750ed00d564b ***
   [03:22:37.323,028] 0m<inf> mspi_ambiq_timing_scan: TxNeg=0, RxNeg=0, RxCap=0, Turnaround=5
   [03:22:37.918,426] 0m<inf> mspi_ambiq_timing_scan:     TxDQSDelay: 0, RxDQSDelay Scan = 0x0007FFFE, Window size = 18
   [03:22:38.528,900] 0m<inf> mspi_ambiq_timing_scan:     TxDQSDelay: 1, RxDQSDelay Scan = 0x0007FFFF, Window size = 19
   [03:22:38.528,900] 0m<inf> mspi_ambiq_timing_scan:     TxDQSDelay: 1, RxDQSDelay Scan = 0x0007FFFF, Window size = 19
   [03:22:39.126,922] 0m<inf> mspi_ambiq_timing_scan:     TxDQSDelay: 2, RxDQSDelay Scan = 0x0007FFFE, Window size = 18
   [03:22:39.731,353] 0m<inf> mspi_ambiq_timing_scan:     TxDQSDelay: 3, RxDQSDelay Scan = 0x0007FFFF, Window size = 19
   [03:22:40.318,908] 0m<inf> mspi_ambiq_timing_scan:     TxDQSDelay: 4, RxDQSDelay Scan = 0x0007FFFE, Window size = 18
   [03:22:40.829,895] 0m<inf> mspi_ambiq_timing_scan:     TxDQSDelay: 5, RxDQSDelay Scan = 0x0005FD54, Window size = 7
   [03:22:41.172,668] 0m<inf> mspi_ambiq_timing_scan:     TxDQSDelay: 6, RxDQSDelay Scan = 0x00000000, Window size = 0
   [03:22:41.463,714] 0m<inf> mspi_ambiq_timing_scan:     TxDQSDelay: 7, RxDQSDelay Scan = 0x00000000, Window size = 0
   [03:22:41.752,807] 0m<inf> mspi_ambiq_timing_scan:     TxDQSDelay: 8, RxDQSDelay Scan = 0x00000000, Window size = 0
   [03:22:42.040,100] 0m<inf> mspi_ambiq_timing_scan:     TxDQSDelay: 9, RxDQSDelay Scan = 0x00000000, Window size = 0
   [03:22:42.325,469] 0m<inf> mspi_ambiq_timing_scan:     TxDQSDelay: 10, RxDQSDelay Scan = 0x00000000, Window size = 0
   [03:22:42.333,496] 0m<inf> mspi_ambiq_timing_scan: Selected setting: TxNeg=0, RxNeg=0, RxCap=0, Turnaround=5,TxDQSDelay=2, RxDQSDelay=9
