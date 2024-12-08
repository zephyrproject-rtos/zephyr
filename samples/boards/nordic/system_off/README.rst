.. zephyr:code-sample:: nrf_system_off
   :name: System Off
   :relevant-api: sys_poweroff subsys_pm_device

   Use deep sleep on Nordic platforms.

Overview
********

This sample can be used for basic power measurement and as an example of
deep sleep on Nordic platforms.

RAM Retention
=============

This sample can also can demonstrate RAM retention. By selecting
``CONFIG_APP_USE_NRF_RETENTION=y`` or ``CONFIG_APP_USE_RETAINED_MEM=y``
state related to number of boots, number of times system off was entered,
and total uptime since initial power-on are retained in a checksummed data structure.
RAM is configured to keep the containing section powered while in system-off mode.

Requirements
************

This application uses nRF51 DK, nRF52 DK or nRF54L15 DK board for the demo.

Sample Output
=============

nRF52 core output
-----------------

.. code-block:: console

   *** Booting Zephyr OS build v2.3.0-rc1-204-g5f2eb85f728d  ***

   nrf52dk system off demo
   Entering system off; press sw0 to restart
