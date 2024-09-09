.. zephyr:code-sample:: nrf-coresight-stm
   :name: Nordic Coresight STM logging
   :relevant-api: logging

   Nordic Coresight STM logging sample.

Overview
********

This sample demonstrates use of :ref:`logging_cs_stm`. Logging can work in two modes:
standalone and dictionary-based. Standalone mode outputs human readable log messages
on UART and dictionary-based logging requires host tool to decode the data from UART.
Sample measures logging performance by logging the same messages multiple times and
calculating average time needed to log a given message type.

Requirements
************

This application uses nRF54H20 DK.
