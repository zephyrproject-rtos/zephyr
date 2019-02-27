.. _ptp_time_interface:


Precision Time Protocol (PTP) time format
#########################################

.. contents::
    :local:
    :depth: 2

Overview
********

The PTP time struct can store time information in high precision
format (nanoseconds). The extended timestamp format can store the
time in fractional nanoseconds accuracy. The PTP time format is used
in :ref:`gptp_interface` implementation.

API Reference
*************

.. doxygengroup:: ptp_time
   :project: Zephyr
