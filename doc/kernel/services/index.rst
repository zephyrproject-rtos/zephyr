.. _kernel_api:

Kernel Services
###############

The Zephyr kernel lies at the heart of every Zephyr application. It provides
a low footprint, high performance, multi-threaded execution environment
with a rich set of available features. The rest of the Zephyr ecosystem,
including device drivers, networking stack, and application-specific code,
uses the kernel's features to create a complete application.

The configurable nature of the kernel allows you to incorporate only those
features needed by your application, making it ideal for systems with limited
amounts of memory (as little as 2 KB!) or with simple multi-threading
requirements (such as a set of interrupt handlers and a single background task).
Examples of such systems include: embedded sensor hubs, environmental sensors,
simple LED wearable, and store inventory tags.

Applications requiring more memory (50 to 900 KB), multiple communication
devices (like Wi-Fi and Bluetooth Low Energy), and complex multi-threading,
can also be developed using the Zephyr kernel. Examples of such systems
include: fitness wearables, smart watches, and IoT wireless gateways.

Scheduling, Interrupts, and Synchronization
*******************************************

These pages cover basic kernel services related to thread scheduling and
synchronization.

.. toctree::
   :maxdepth: 1

   threads/index.rst
   scheduling/index.rst
   threads/system_threads.rst
   threads/workqueue.rst
   threads/nothread.rst
   interrupts.rst
   polling.rst
   synchronization/semaphores.rst
   synchronization/mutexes.rst
   synchronization/condvar.rst
   synchronization/events.rst
   smp/smp.rst

.. _kernel_data_passing_api:

Data Passing
************

These pages cover kernel objects which can be used to pass data between
threads and ISRs.

The following table summarizes their high-level features.

===============   ==============      ===================    ================    =================   =================  ==============  ===============================
Object            Bidirectional?      Data structure         Data item size      Data Alignment      ISRs can receive?  ISRs can send?  Overrun handling
===============   ==============      ===================    ================    =================   =================  ==============  ===============================
FIFO              No                  Queue                  Arbitrary [#f1]_    4 B [#f2]_          Yes [#f3]_         Yes             N/A
LIFO              No                  Queue                  Arbitrary [#f1]_    4 B [#f2]_          Yes [#f3]_         Yes             N/A
Stack             No                  Array                  Word                Word                Yes [#f3]_         Yes             Undefined behavior
Message queue     No                  Ring buffer            Arbitrary [#f6]_    Power of two        Yes [#f3]_         Yes             Pend thread or return -errno
Mailbox           Yes                 Queue                  Arbitrary [#f1]_    Arbitrary           No                 No              N/A
Pipe              No                  Ring buffer [#f4]_     Arbitrary           Arbitrary           Yes [#f5]_         Yes [#f5]_      Pend thread or return -errno
===============   ==============      ===================    ================    =================   =================  ==============  ===============================

.. rubric:: Footnotes

.. [#f1] Callers allocate space for queue overhead in the data
         elements themselves.

.. [#f2] Objects added with :c:func:`k_fifo_alloc_put()` and :c:func:`k_lifo_alloc_put()`
         do not have alignment constraints, but use temporary memory from the
         calling thread's resource pool.

.. [#f3] ISRs can receive only when passing K_NO_WAIT as the timeout
         argument.

.. [#f4] Optional.

.. [#f5] ISRs can send and/or receive only when passing K_NO_WAIT as the
         timeout argument.

.. [#f6] Data item size must be a multiple of the data alignment.

.. toctree::
   :maxdepth: 1

   data_passing/queues.rst
   data_passing/fifos.rst
   data_passing/lifos.rst
   data_passing/stacks.rst
   data_passing/message_queues.rst
   data_passing/mailboxes.rst
   data_passing/pipes.rst

.. _kernel_memory_management_api:

Memory Management
*****************

See :ref:`memory_management_api`.

Timing
******

These pages cover timing related services.

.. toctree::
   :maxdepth: 1

   timing/clocks.rst
   timing/timers.rst

Other
*****

These pages cover other kernel services.

.. toctree::
   :maxdepth: 1

   other/atomic.rst
   other/float.rst
   other/version.rst
   other/fatal.rst
   other/thread_local_storage.rst
