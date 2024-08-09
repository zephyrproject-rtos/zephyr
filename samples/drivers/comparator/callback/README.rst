.. zephyr:code-sample:: comparator_callback
   :name: Comparator callback
   :relevant-api: comparator_interface

   Monitor the output of a comparator

Overview
********

This sample demonstrates how to monitor the output from a comparator
using the comparator device driver API.

Requirements
************

This sample requires a board with a comparator device present and enabled
in the devicetree. The comparator shall be pointed to by the devicetree alias
``comparator``.

Sample Output
*************

.. code-block:: console

   comparator output is high
   comparator output is low
   comparator output is high
   comparator output is low
