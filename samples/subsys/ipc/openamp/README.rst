.. _openAMP_sample:

OpenAMP Sample Application
##########################

Overview
********

This application demonstrates how to use OpenAMP with Zephyr. It is designed to
demonstrate how to integrate OpenAMP with Zephyr both from a build perspective
and code.  Currently this integration is specific to the LPC54114 SoC.

Building the application
*************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/openamp
   :board: lpcxpresso54114_m4
   :goals: debug

Open a serial terminal (minicom, putty, etc.) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and the following message will appear on the corresponding
serial port:

.. code-block:: console

   ***** Booting Zephyr OS v1.11.0-1377-g580b9add47 *****
   Starting application thread!

   OpenAMP demo started
   Primary core received a message: 1
   Primary core received a message: 3
   Primary core received a message: 5
   ...
   Primary core received a message: 101
   OpenAMP demo ended.
