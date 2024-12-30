.. _m85:

TSI Banner
###########

Overview
********

This is the core M85 Zephyr platform and initial startup code. It prints TSI Banner to the console by invocation of Zephyr main. This module also enables the logging functionality  to log messages to console at different levels of severity.

Building and Running
********************

This application can be built as follows:
.. zephyr-app-commands::
   :zephyr-app:
   :host-os:unix
   :board:tsi/skyp/m85
   :goals: run
   :compact:

Sample Output
=============
.. code-block:: console
*** Booting Zephyr OS build v3.7.0-rc1-537-g582e2b7691eb ***
        Tsavorite Scalable Intelligence

    |||||||||||||||||||||||||||||||||||||
    |||||||||||||||||||||||||||||||||||||
    ||||||                          |||||
    ||||||                          |||||
    |||||||||||||||||   |||||       |||||
    |||||||||||||||||   |||||       |||||
               ||||||   |||||
    ||||||     ||||||   |||||      ||||||
     ||||||||  ||||||   |||||   ||||||||
       ||||||||||||||   ||||||||||||||
          |||||||||||   ||||||||||||
            |||||||||   ||||||||||
              |||||||   ||||||||
                |||||   |||||
                  |||   |||
                        |
[00:00:00.012 000] <inf> m85: Test Platform: tsi/skyp/m85
[00:00:00.013 000] <wrn> m85: Testing on FPGA
TSI Logging enabled and printk is functional
TSI:~$
TSI:~$

