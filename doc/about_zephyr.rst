.. _about_zephyr::

About Zephyr
############

The Zephyr kernel is a small footprint kernel designed for use on
resource-constrained systems, such as embedded sensor hubs,
environmental sensors, simple LED wearables, and store inventory tags.

Key Features
************

A Zephyr application consists of kernel code, device driver code, and
application-specific code, which are compiled into a monolithic image
executing in a single shared address space.

The Zephyr kernel provides an extensive suite of services,
which are summarized below.

* Multi-threading services, including both priority-based, non-preemptive fibers
  and priority-based, preemptive tasks (with optional round robin time-slicing).

* Interrupt services, including both compile-time and run-time registration
  of interrupt handlers, which can be written in C or assembly language.

* Inter-thread signalling services, including binary sempahores,
  counting semaphores, and mutex semaphores.

* Inter-thread data passing services, including basic message queues,
  enhanced message queues, and byte streams.

* Memory management services, including dynamic allocation and freeing of
  fixed-size or variable-size memory blocks.

* Power management services, including tickless idle and an advanced idling
  infrastructure.

There are several additional features that distinguish Zephyr from
other small footprint kernels.

* Zephyr is highly configurable, allowing an application to incorporate only
  the capabilities it needs, and to specify their quantity and size.

* Zephyr requires all system resources to be defined at compile-time
  to reduce code size and increase performance.

* Zephyr provides minimal run-time error checking to reduce code size and
  increase performance. An optional error checking infrastructure is provided
  that can assist in debugging during application development.

The Zephyr kernel is suppported on multiple architectures,
including ARM Cortex-M, Intel x86, and ARC. The list of supported platforms
can be found :ref:`here <platform>`.
