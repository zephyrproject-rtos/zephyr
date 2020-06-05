.. _peci_api:

PECI
####

Overview
********
The Platform Environment Control Interface, abbreviated as PECI,
is a thermal management standard introduced in 2006
with the Intel Core 2 Duo Microprocessors.
The PECI interface allows external devices to read processor temperature,
perform processor manageability functions, and manage processor interface
tuning and diagnostics.
The PECI bus driver APIs enable the interaction between Embedded
Microcontrollers and CPUs.

Configuration Options
*********************

Related configuration options:

* :option:`CONFIG_PECI`

API Reference
*************

.. doxygengroup:: peci_interface
   :project: Zephyr
