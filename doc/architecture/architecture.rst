.. _technical_overview:

The Zephyr Kernel
#################

The |codename| is composed of a microkernel operating on top of a nanokernel.
 Device drivers must operate on top of either a microkernel or a nanokernel
 and must be developed accordingly. Applications can be developed to operate
 on top of a nanokernel, a microkernel or both.

The |codename| provides three scheduling or execution contexts. The
:abbr:`ISRs (Interrupt Service Routines)` are the execution context closest to the
hardware. They are followed by the fibers execution context. Finally, the
tasks comprise the third and last execution context.

ISRs can interrupt fibers and tasks. By default, it is possible to nest ISRs
but that option can be disabled. The main purpose of ISRs is to mark fibers
and tasks as runnable.

Fibers are typically used for device drivers and performance critical work.
They are cooperatively scheduled and each fiber possesses a priority. Fibers
run until they yield or call a blocking
:abbr:`API (Application Program Interface)`. Once a fiber is marked as not runnable, the next highest
priority fiber runs.

Tasks are used, primarily for data processing. A task is scheduled only when
no fibers are marked as runnable. Tasks can be preempted and the highest
priority task runs first. Tasks of equal priority use round-robin time-
slicing between them.

The following sections describe the general functionality and basic
characteristics of the nanokernel and the microkernel as well as the
principles behind the development of drivers and applications.

.. toctree::
   :maxdepth: 3

   architecture_nanokernel
   architecture_microkernel
   architecture_drivers
   architecture_apps