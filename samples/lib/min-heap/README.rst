.. zephyr:code-sample:: min-heap
   :name: Min-Heap Data Structure

   Demonstrate usage of a min-heap implementation in a Zephyr application.

Overview
********

This sample demonstrates Min-Heap Data Structure implementation used as a
priority queue in a Zephyr application.
The example shows basic heap operations such as insert, remove, pop and
empty check.

Building and Running
********************

To build and run this sample on a supported board:

.. code-block:: console

   west build -b <your_board> samples/lib/min_heap
   west flash

Replace ``<your_board>`` with your actual board name (e.g., ``native_sim``).

Sample Output
*************

On startup, the sample application will perform a sequence of heap operations
and print the results. Expected output resembles:

.. code-block:: console

   *** Booting Zephyr OS build 2ffbd869266c ***
   [00:00:00.000,000] <inf> min_heap_app: Top node key value 2
   [00:00:00.000,000] <inf> min_heap_app: Removed node key value 7
   [00:00:00.000,000] <inf> min_heap_app: Heap is not empty

Requirements
************

No external hardware is required to run this sample.
It runs on any Zephyr-supported board with standard console output or
native_sim.
