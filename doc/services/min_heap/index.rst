.. _min_heap:

Minimum Heap (Min-heap)
#######################

.. contents::
  :local:
  :depth: 2

The Minimum Heap implementation provides an efficient data structure for
managing a dynamically changing list of elements while maintaining the ability
to quickly extract the minimum value. It follows the binary heap property and
supports common operations such as insertion, removal, and popping the
minimum element and destroying the min-heap.

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

Example
*******

.. code-block:: c

        #include <zephyr/min_heap.h>
        #include <zephyr/logging/log.h>

        #define MAX_ITEMS 3

        LOG_MODULE_REGISTER(min_heap_app);

        struct data {
                struct min_heap_node node;
                const char *name;
        };

        static struct min_heap_node *heap_buf[MAX_ITEMS];
        static struct min_heap heap;

        static struct data items[MAX_ITEMS] = {
                { .node.key = 1, .name = "Task1" },
                { .node.key = 2, .name = "Task2" },
                { .node.key = 3, .name = "Task3" }
        };

        static int compare(const struct min_heap_node *a,
                           const struct min_heap_node *b)
        {
                return a->key - b->key;
        }

        int main (void) {

                int ret = 0;

                ret = min_heap_init(&heap, heap_buf, MAX_ITEMS, compare);
                if (ret != 0) {
                        LOG_ERR("min-heap init failure %d", ret);
                        return ret;
                }

                for (int i = 0; i < MAX_ITEMS; i++) {

                        ret = min_heap_insert(&heap, &items[i].node);
                        if (ret != 0) {
                                LOG_ERR("min-heap insert failure %d", ret);
                                return ret;
                        }
                }

                struct min_heap_node *node = min_heap_pop(&heap);

                if (node == NULL) {
                        LOG_ERR("min-heap empty or invalid heap");
                        return -1;
                }

                LOG_INF("min-heap top key value %d", node->key);

                ret = min_heap_destroy(&heap);
                if (ret != 0) {
                        LOG_ERR("Invalid heap %d", ret);
                return ret;
                }

                return 0;
        }

API Reference
*************

.. doxygengroup:: min_heap
