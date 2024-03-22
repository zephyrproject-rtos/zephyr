.. _nrf-system-off-sample:

nRF5x System Off demo
#####################

Overview
********

This sample can be used for basic power measurement and as an example of
deep sleep on Nordic platforms.

RAM Retention
=============

On nRF52 platforms this also can demonstrate RAM retention.  By selecting
``CONFIG_APP_RETENTION=y`` state related to number of boots, number of times
system off was entered, and total uptime since initial power-on are retained
in a checksummed data structure.  The POWER peripheral is configured to keep
the containing RAM section powered while in system-off mode.

Requirements
************

This application uses nRF51 DK or nRF52 DK board for the demo.

Sample Output
=============

nRF52 core output
-----------------

.. code-block:: console

   *** Booting Zephyr OS build v2.3.0-rc1-204-g5f2eb85f728d  ***

   nrf52dk system off demo
   Entering system off; press sw0 to restart
