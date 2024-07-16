.. zephyr:code-sample:: comparator_pending
   :name: Comparator pending
   :relevant-api: comparator_interface

   Monitor the output of a comparator

Overview
********

This sample demonstrates how to enable the comparator trigger and
periodically check and clear the trigger pending flag using the
comparator device driver API.

Requirements
************

This sample requires a board with a comparator device present and enabled
in the devicetree. The comparator shall be pointed to by the devicetree alias
``comparator``.

Sample Output
*************

.. code-block:: console

   comparator trigger is not pending
   comparator trigger is not pending
   comparator trigger is not pending
   comparator trigger is not pending
   comparator trigger is pending
   comparator trigger is not pending
   comparator trigger is not pending
