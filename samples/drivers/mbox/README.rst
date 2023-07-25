.. _mbox-sample:

MBOX Interface
##############

Overview
********

This sample demonstrates how to use the :ref:`MBOX API <mbox_api>`.

Building and Running
********************

The sample can be built and executed on boards supporting MBOX.

Building the application for nrf5340dk_nrf5340_cpuapp
*****************************************************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/mbox/
   :board: nrf5340dk_nrf5340_cpuapp
   :goals: debug

Open a serial terminal (minicom, putty, etc.) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and the following message will appear on the corresponding
serial port, one is the application (APP) core another is the network (NET)
core:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.1.0-2383-g147aee22f273  ***
   Hello from APP
   Pong (on channel 0)
   Ping (on channel 1)
   Pong (on channel 0)
   Ping (on channel 1)
   Ping (on channel 1)
   Pong (on channel 0)
   Ping (on channel 1)
   Pong (on channel 0)
   Ping (on channel 1)
   ...


.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.1.0-2383-g147aee22f273  ***
   Hello from NET
   Ping (on channel 0)
   Pong (on channel 1)
   Ping (on channel 0)
   Pong (on channel 1)
