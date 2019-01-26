.. _kernel:

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
devices (like WiFi and Bluetooth Low Energy), and complex multi-threading,
can also be developed using the Zephyr kernel. Examples of such systems
include: fitness wearables, smart watches, and IoT wireless gateways.


.. toctree::
   :maxdepth: 2

   threads/index.rst
   other/interrupts.rst
   usermode/usermode.rst
   synchronization/semaphores.rst
   synchronization/mutexes.rst
   data_passing/fifos.rst
   data_passing/lifos.rst
   data_passing/stacks.rst
   data_passing/message_queues.rst
   data_passing/mailboxes.rst
   data_passing/pipes.rst
   timing/clocks.rst
   other/cpu_idle.rst
   timing/timers.rst
   memory/slabs.rst
   memory/pools.rst
   memory/heap.rst
   other/atomic.rst
   other/polling.rst
   other/float.rst
   other/ring_buffers.rst
   other/cxx_support.rst
   other/version.rst
