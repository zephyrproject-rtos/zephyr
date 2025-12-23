.. zephyr:code-sample:: red-black-tree
   :name: Red-Black Tree Data Structure

   Use a red-black tree data structure in a Zephyr application.

Overview
********

This sample demonstrates how to use the Red-Black Tree (rbtree) data structure
in a Zephyr application.
The example shows basic rbtree operations such as insert, remove and foreach.

Building and Running
********************

To build and run this sample on a supported board:

.. code-block:: console

   west build -b <your_board> samples/data_structures/rbtree
   west flash

Replace ``<your_board>`` with your actual board name (e.g., ``native_sim``).

Sample Output
*************

On startup, the sample application will perform a sequence of rbtree operations
and print the results. Expected output resembles:

.. code-block:: console

        *** Booting Zephyr OS build gd2a5c1ca82f0 ***
        insert n=1 rb=0x105004
        insert n=3 rb=0x105010
        insert n=2 rb=0x10501c
        insert n=5 rb=0x105028
        insert n=4 rb=0x105034
        max=1 min=5
        rbtree elements:
        n=5
        n=4
        n=3
        n=2
        n=1
        removed max=1
        removed min=5
        rbtree after removal:
        n=4
        n=3
        n=2
        Done


Requirements
************

No external hardware is required to run this sample.
It runs on any Zephyr-supported board with standard console output, or
native_sim.
