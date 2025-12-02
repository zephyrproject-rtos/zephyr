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

   west build -b <your_board> samples/data_structures/min_heap
   west flash

Replace ``<your_board>`` with your actual board name (e.g., ``native_sim``).

Sample Output
*************

On startup, the sample application will perform a sequence of heap operations
and print the results. Expected output resembles:

.. code-block:: console

        *** Booting Zephyr OS build 9c0c063db09d ***
        Min-heap sample using static storage
        Heap elements by order of priority:
        key=2 value=400
        key=5 value=200
        key=30 value=300
        key=10 value=100
        Top of heap: key=2 value=400
        Found element with key 5 at index 1,removing it...
        Heap after removal:
        key=2 value=400
        key=10 value=100
        key=30 value=300



Requirements
************

No external hardware is required to run this sample.
It runs on any Zephyr-supported board with standard console output or
native_sim.
