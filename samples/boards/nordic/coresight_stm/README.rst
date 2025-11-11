.. zephyr:code-sample:: coresight_stm_sample
   :name: Coresight STM benchmark
   :relevant-api: log_api

Overview
********

This sample presents how to enable STM logging on nRF54H20 platform.

Also, it prints timing for different log messages.
Thus, performance of different loggers can be compared.

There are three sample configurations in the :file:`sample.yaml`.

* **sample.boards.nrf.coresight_stm.local_uart**

  This configuration doesn't use STM.
  Logs are printed on local console.

* **sample.boards.nrf.coresight_stm**

  This configuration use STM.
  Application, Radio, PPR and FLPR cores send logs to an ETR buffer.
  Proxy (Application core) gets logs from the ETR buffer, decodes STPv2 data
  and prints human readable logs on UART.

* **sample.boards.nrf.coresight_stm.dict**

  This sample uses STM logging in dictionary mode.
  Application, Radio, PPR and FLPR cores send logs to the ETR buffer.
  Proxy (Application core) forwards data from the ETR to the host using UART.
  Host tool is needed to decode the logs.

Requirements
************

This application uses nRF54H20 DK board for the demo.

**sample.boards.nrf.coresight_stm.dict** requires host tool like ``nrfutil trace``
to decode the traces.

Building and running
********************

To build the sample, use configuration setups from the :file:`sample.yaml` using the ``-T`` option.
See the example:

nRF54H20 DK

  .. code-block:: console

     west build -p -b nrf54h20dk/nrf54h20/cpuapp -T sample.boards.nrf.coresight_stm .

Sample Output
=============

.. code-block:: console

   *** Using Zephyr OS v3.6.99-5bb7bb0af17c ***
   (...)
   [00:00:00.227,264] <inf> app/app: test with one argument 100
   [00:00:00.227,265] <inf> app/app: test with one argument 100
   (...)
   [00:00:00.585,558] <inf> rad/app: test with one argument 100
   [00:00:00.585,569] <inf> rad/app: test with one argument 100
   (...)
   [00:00:00.624,408] <inf> ppr/app: test with one argument 100
   [00:00:00.624,433] <inf> ppr/app: test with one argument 100
   (...)
   [00:00:00.625,249] <inf> flpr/app: test with one argument 100
   [00:00:00.625,251] <inf> flpr/app: test with one argument 100
   (...)
   rad: Timing for log message with 0 arguments: 5.10us
   rad: Timing for log message with 1 argument: 6.10us
   rad: Timing for log message with 2 arguments: 6.0us
   rad: Timing for log message with 3 arguments: 6.40us
   rad: Timing for log_message with string: 7.10us
   rad: Timing for tracepoint: 0.5us
   rad: Timing for tracepoint_d32: 0.5us
   flpr: Timing for log message with 0 arguments: 1.20us
   flpr: Timing for log message with 1 argument: 1.20us
   flpr: Timing for log message with 2 arguments: 1.20us
   flpr: Timing for log message with 3 arguments: 1.30us
   flpr: Timing for log_message with string: 3.0us
   flpr: Timing for tracepoint: 0.0us
   flpr: Timing for tracepoint_d32: 0.0us
   app: Timing for log message with 0 arguments: 1.80us
   app: Timing for log message with 1 argument: 2.0us
   app: Timing for log message with 2 arguments: 2.0us
   app: Timing for log message with 3 arguments: 2.10us
   app: Timing for log_message with string: 4.40us
   app: Timing for tracepoint: 0.10us
   app: Timing for tracepoint_d32: 0.10us
   ppr: Timing for log message with 0 arguments: 25.20us
   ppr: Timing for log message with 1 argument: 26.20us
   ppr: Timing for log message with 2 arguments: 26.90us
   ppr: Timing for log message with 3 arguments: 27.40us
   ppr: Timing for log_message with string: 64.80us
   ppr: Timing for tracepoint: 0.30us
   ppr: Timing for tracepoint_d32: 0.25us

For logging on NRF54H20 using ARM Coresight STM see :ref:`logging_cs_stm`.
