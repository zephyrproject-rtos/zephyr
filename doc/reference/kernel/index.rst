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
   other/interrupts.rst
   other/polling.rst
   synchronization/semaphores.rst
   synchronization/mutexes.rst
   smp/smp.rst

Data Passing
************

These pages cover kernel objects which can be used to pass data between
threads and ISRs.

The following table summarizes their high-level features.

===============   ==============      ===================    ==============      ==============   =================  ==============  ===============================
Object            Bidirectional?      Data structure         Data item size      Data Alignment   ISRs can receive?  ISRs can send?  Overrun handling
===============   ==============      ===================    ==============      ==============   =================  ==============  ===============================
FIFO              No                  Queue                  Arbitrary [1]              4 B [2]   Yes [3]            Yes             N/A
LIFO              No                  Queue                  Arbitrary [1]              4 B [2]   Yes [3]            Yes             N/A
Stack             No                  Array                  Word                          Word   Yes [3]            Yes             Undefined behavior
Message queue     No                  Ring buffer            Power of two          Power of two   Yes [3]            Yes             Pend thread or return -errno
Mailbox           Yes                 Queue                  Arbitrary [1]            Arbitrary   No                 No              N/A
Pipe              No                  Ring buffer [4]        Arbitrary                Arbitrary   No                 No              Pend thread or return -errno
===============   ==============      ===================    ==============      ==============   =================  ==============  ===============================

[1] Callers allocate space for queue overhead in the data
elements themselves.

[2] Objects added with k_fifo_alloc_put() and k_lifo_alloc_put()
do not have alignment constraints, but use temporary memory from the
calling thread's resource pool.

[3] ISRs can receive only when passing K_NO_WAIT as the timeout
argument.

[4] Optional.

.. toctree::
   :maxdepth: 1

   data_passing/queues.rst
   data_passing/fifos.rst
   data_passing/lifos.rst
   data_passing/stacks.rst
   data_passing/message_queues.rst
   data_passing/mailboxes.rst
   data_passing/pipes.rst

Memory Management
*****************

These pages cover memory allocation and management services.

.. toctree::
   :maxdepth: 1

   memory/heap.rst
   memory/slabs.rst
   memory/pools.rst

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

   other/cpu_idle.rst
   other/atomic.rst
   other/float.rst
   other/ring_buffers.rst
   other/cxx_support.rst
   other/version.rst
   other/fatal.rst
