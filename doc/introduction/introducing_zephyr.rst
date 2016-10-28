.. _introducing_zephyr:

Introducing Zephyr
##################

The Zephyr kernel is a small-footprint kernel designed for use on
resource-constrained systems: from simple embedded environmental
sensors and LED wearables to sophisticated smart watches and IoT
wireless gateways.

It is designed to be supported by multiple architectures, including
ARM Cortex-M, Intel x86, and ARC. The full list of supported boards
can be found :ref:`here <board>`.

Licensing
*********

The Zephyr project associated with the kernel makes it available
to users and developers under the Apache License, version 2.0.

Distinguishing Features
***********************

The Zephyr kernel offers a number of features that distinguish it from other
small-footprint OSes:

#. **Single address-space**. Combines application-specific code
   with a custom kernel to create a monolithic image that gets loaded
   and executed on a system's hardware. Both the application code and
   kernel code execute in a single shared address space.

#. **Highly configurable**. Allows an application to incorporate *only*
   the capabilities it needs as it needs them, and to specify their
   quantity and size.

#. **Compile-time resource definition**. Allows system resources
   to be defined at compile-time, which reduces code size and
   increases performance.

#. **Minimal error checking**. Provides minimal run-time error checking
   to reduce code size and increase performance. An optional error-checking
   infrastructure is provided to assist in debugging during application
   development.

#. **Extensive suite of services**. Offers a number of familiar services
   for development:

   * *Multi-threading Services* for priority-based, non-preemptive and
     preemptive threads with optional round robin time-slicing.

   * *Interrupt Services* for compile-time registration of interrupt handlers.

   * *Memory Allocation Services* for dynamic allocation and freeing of
     fixed-size or variable-size memory blocks.

   * *Inter-thread Synchronization Services* for binary semaphores,
     counting semaphores, and mutex semaphores.

   * *Inter-thread Data Passing Services* for basic message queues, enhanced
     message queues, and byte streams.

   * *Power Management Services* such as tickless idle and an advanced idling
     infrastructure.

Fundamental Terms and Concepts
******************************

This section outlines the basic terms used by the Zephyr kernel ecosystem.

:dfn:`kernel`
   The set of Zephyr-supplied files that implement the Zephyr kernel,
   including its core services, device drivers, network stack, and so on.

:dfn:`application`
   The set of user-supplied files that the Zephyr build system uses
   to build an application image for a specified board configuration.
   It can contain application-specific code, kernel configuration settings,
   and at least one Makefile.

   The application's kernel configuration settings direct the build system
   to create a custom kernel that makes efficient use of the board's resources.

   An application can sometimes be built for more than one type of board
   configuration (including boards with different CPU architectures),
   if it does not require any board-specific capabilities.

:dfn:`application image`
   A binary file that is loaded and executed by the board for which
   it was built.

   Each application image contains both the application's code and the
   Zephyr kernel code needed to support it. They are compiled as a single,
   fully-linked binary.

   Once an application image is loaded onto a board, the image takes control
   of the system, initializes it, and runs as the system's sole application.
   Both application code and kernel code execute as privileged code
   within a single shared address space.

:dfn:`board`
   A target system with a defined set of devices and capabilities,
   which can load and execute an application image. It may be an actual
   hardware system or a simulated system running under QEMU.

   The Zephyr kernel supports a :ref:`variety of boards <board>`.

:dfn:`board configuration`
   A set of kernel configuration options that specify how the devices
   present on a board are used by the kernel.

   The Zephyr build system defines one or more board configurations
   for each board it supports. The kernel configuration settings that are
   specified by the build system can be over-ridden by the application,
   if desired.
