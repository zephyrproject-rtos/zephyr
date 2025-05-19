.. _min_heap:

Min-Heap Data Structure
#######################

.. contents::
  :local:
  :depth: 2

The Min-Heap implementation provides an efficient data structure for
managing a dynamically changing list of elements while maintaining the ability
to quickly extract the minimum value. It's a tree-based data structure that
satisfies the heap property and supports common operations such as insertion,
removal and popping the minimum element from the Min-Heap

This section explains the motivation behind the implementation, its internal
structure, API usage, and example scenarios for embedded systems and real-time
environments.

Heap Structure
**************

The heap is maintained as a complete binary tree stored in an array.
Each node satisfies the **min-heap** property:

   - The value of each node is less than or equal to the values of its children.

This property ensures that the **minimum element is always at the root**
(index 0).

.. code-block:: text

    Index:      0   1   2   3   4   5   6
    Values:    [1,  3,  5,  8,  9, 10, 12]

                   1
                 /   \
               3       5
              / \     / \
             8   9  10  12

For any node at index ``i``, its children are at indices:
- Left child:  ``2*i + 1``
- Right child: ``2*i + 2``

Its parent is at index:
- Parent: ``(i - 1) / 2``

Use Cases
*********

MinHeap is well suited for:

- Implementing priority queues
- Sorting data incrementally
- Resource arbitration (e.g., lowest-cost resource selection)
- Scheduling in cooperative multitasking systems
- Managing timeouts and delay queues
- Priority-based sensor or data sampling

In RTOS environments like Zephyr, this heap can be used in kernel-level or
application-level modules where minimal latency to obtain the highest priority
resource is needed.

API Reference
*************

.. doxygengroup:: min_heap
