.. _about_zephyr::

About
#####

The |codename| is a small-footprint kernel designed for use on
resource-constrained systems: from simple embedded environmental
sensors and LED wearables to sophisticated smart watches and IoT
wireless gateways.

It is designed to be supported by multiple architectures, including
ARM Cortex-M, Intel x86, and ARC. The full list of supported platforms
can be found :ref:`here <platform>`.

Licensing
*********

The Zephyr project associated with the kernel makes it available
to users and developers under the Apache License, version 2.0.

Distinguishing Features
***********************

The |codename| offers a number of features that distinguish it from other
small-footprint OSes:

#. **Single address-space OS**. Combines application-specific code
   with a custom kernel to create a monolithic image that gets loaded
   and executed on a system's hardware. Both the application code and
   kernel code execute in a single shared address space.

#. **Highly configurable**. Allows an application to incorporate *only*
   the capabilities it needs as it needs them, and to specify their
   quantity and size.

#.  **Resources defined at compile-time**. Requires all system resources
   be defined at compilation time, which reduces code size and
   increases performance.

#. **Minimal error checking**. Provides minimal run-time error checking
   to reduce code size and increase performance. An optional error-checking
   infrastructure is provided to assist in debugging during application
   development.

#. **Extensive suite of services** Offers a number of familiar services
   for development:

    * *Multi-threading Services* for both priority-based, non-preemptive
      fibers and priority-based, preemptive tasks with optional round robin
      time-slicing.

    * *Interrupt Services* for both compile-time and run-time registration
      of interrupt handlers.

    * *Inter-thread Synchronization Services* for binary sempahores,
      counting semaphores, and mutex semaphores.

   * *Inter-thread Data Passing Services* for basic message queues, enhanced
     message queues, and byte streams.

   * *Memory Allocation Services* for dynamic allocation and freeing of
     fixed-size or variable-size memory blocks.

   * *Power Management Services* such as tickless idle and an advanced idling
     infrastructure.