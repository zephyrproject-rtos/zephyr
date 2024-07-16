.. zephyr:code-sample:: comparator
   :name: Comparator
   :relevant-api: comparator_interface

   Use comparator trigger as wakeup source

Overview
********

This sample demonstrates how to prepare a comparator to wake up the
system when the comparator triggers using the comparator device driver API
and ``sys_poweroff()``

Requirements
************

This sample requires a board with a comparator device present and enabled
in the devicetree, which is capable of waking up the system from poweroff.
The comparator shall be pointed to by the devicetree alias ``comparator``.

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build v3.7.0-rc2-381-g01f34269b77d ***
   delay before poweroff
   check if trigger is pending to clear it
   comparator trigger was not pending

Crossing positive and negative inputs

.. code-block:: console

   *** Booting Zephyr OS build v3.7.0-rc2-381-g01f34269b77d ***
   delay before poweroff
   check if trigger is pending to clear it
   comparator trigger was not pending
