.. _data_structures:

Data Structures
###############

Zephyr provides a library of common general purpose data structures
used within the kernel, but useful by application code in general.
These include list and balanced tree structures for storing ordered
data, and a ring buffer for managing "byte stream" data in a clean
way.

Note that in general, the collections are implemented as "intrusive"
data structures.  The "node" data is the only struct used by the
library code, and it does not store a pointer or other metadata to
indicate what user data is "owned" by that node.  Instead, the
expectation is that the node will be itself embedded within a
user-defined struct.  Macros are provided to retrieve a user struct
address from the embedded node pointer in a clean way.  The purpose
behind this design is to allow the collections to be used in contexts
where dynamic allocation is disallowed (i.e. there is no need to
allocate node objects because the memory is provided by the user).

Note also that these libraries are uniformly unsynchronized; access to
them is not threadsafe by default.  These are data structures, not
synchronization primitives.  The expectation is that any locking
needed will be provided by the user.

.. toctree::
  :maxdepth: 1

  slist.rst
  dlist.rst
  mpsc_pbuf.rst
  spsc_pbuf.rst
  rbtree.rst
  ring_buffers.rst
