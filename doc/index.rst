..
    Zephyr Project documentation master file

Zephyr Project Documentation
############################

.. only:: release

   Welcome to the Zephyr Project's documentation version |version|!

   Documentation for the development branch of Zephyr can be found at
   https://www.zephyrproject.org/doc/

.. only:: development

   Welcome to the Zephyr Project's documentation. This is the documentation of the
   master tree under development.

   Documentation for released versions of Zephyr can be found at
   ``https://www.zephyrproject.org/doc/<version>``. The following documentation
   versions are vailable:

   `Zephyr 1.1.0`_ | `Zephyr 1.2.0`_ | `Zephyr 1.3.0`_ | `Zephyr 1.4.0`_

Sections
********

.. toctree::
   :maxdepth: 1

   getting_started/getting_started.rst
   board/board.rst
   kernel/kernel.rst
   drivers/drivers.rst
   subsystems/subsystems.rst
   api/api.rst
   application/application.rst
   contribute/code.rst
   porting/porting.rst
   reference/kbuild/kbuild.rst

You can find further information on the `Zephyr Project Wiki`_.

.. _about_zephyr:

Introduction to the Zephyr Project
##################################

The Zephyr Kernel is a small-footprint kernel designed for use on
resource-constrained systems: from simple embedded environmental
sensors and LED wearables to sophisticated smart watches and IoT
wireless gateways.

It is designed to be supported by multiple architectures, including
ARM Cortex-M, Intel x86, and ARC. The full list of supported platforms
can be found :ref:`here <board>`.

Licensing
*********

The Zephyr project associated with the kernel makes it available
to users and developers under the Apache License, version 2.0.

Distinguishing Features
***********************

The Zephyr Kernel offers a number of features that distinguish it from other
small-footprint OSes:

#. **Single address-space OS**. Combines application-specific code
   with a custom kernel to create a monolithic image that gets loaded
   and executed on a system's hardware. Both the application code and
   kernel code execute in a single shared address space.

#. **Highly configurable**. Allows an application to incorporate *only*
   the capabilities it needs as it needs them, and to specify their
   quantity and size.

#. **Resources defined at compile-time**. Requires all system resources
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

   * *Inter-thread Synchronization Services* for binary semaphores,
     counting semaphores, and mutex semaphores.

   * *Inter-thread Data Passing Services* for basic message queues, enhanced
     message queues, and byte streams.

   * *Memory Allocation Services* for dynamic allocation and freeing of
     fixed-size or variable-size memory blocks.

   * *Power Management Services* such as tickless idle and an advanced idling
     infrastructure.

Indices and Tables
******************

* :ref:`genindex`

* :ref:`search`

.. _Zephyr Project Wiki: https://wiki.zephyrproject.org/view/Main_Page
.. _Zephyr 1.4.0: https://www.zephyrproject.org/doc/1.4.0/
.. _Zephyr 1.3.0: https://www.zephyrproject.org/doc/1.3.0/
.. _Zephyr 1.2.0: https://www.zephyrproject.org/doc/1.2.0/
.. _Zephyr 1.1.0: https://www.zephyrproject.org/doc/1.1.0/
