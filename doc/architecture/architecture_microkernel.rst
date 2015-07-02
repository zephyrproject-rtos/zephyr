.. _architecture_microkernel:

The Microkernel
###############

The Zephyr microkernel enhances the nanokernel with multiple tasks that are
scheduled preemptively. The microkernel objects synchronize these tasks and
pass data between them. The microkernel retains the nanokernel with its ISRs
and fibers making it ideal for multiprocessing applications running on
heterogeneous systems.

The microkernel is ideally suited for systems with a midrange of RAM, 50 -
900 KB, multiple communication devices, like WIFI and Bluetooth Low Energy,
and handle multiple data processing tasks. Some devices that could implement
a nanokernel are: fitness wearables, smart watches and IoT wireless gateways.


Microkernel Function
********************

Microkernel applications perform all heavy processing using tasks. Tasks can
implement large blocks of code that perform lengthy processing or complex
functions, for example: programming algorithms, high-level task control
routines or data processing. The available microkernel APIs provide
solutions for traditional kernel functions such as processing
synchronization, data passing, and memory access. The microkernel offers
great flexibility in setting the timing and passing of information for
multitasking applications.

Device drivers and performance-critical parts of the application can still
be implemented as ISRs or fibers, just as in the Zephyr nanokernel, while
other parts can make use of the microkernel tasks, scheduling and APIs.
Thus, ISRs or fibers implemented in standalone nanokernel applications can
easily be rebuilt or re-used in microkernel applications.

Microkernel Objects
*******************

Microkernel tasks can interact with nanokernel objects and, additionally use
microkernel specific objects. The most important microkernel objects are:

* Pipes

* Mailboxes

* Events

* Resource locking objects

   - Memory Maps

   - Memory Pools

The following subsections contain general information about the microkernel
objects. For detailed information see: :ref:`microkernelObjects`.