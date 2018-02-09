.. _introducing_zephyr:

Introducing Zephyr
##################

The Zephyr OS is based on a small-footprint kernel designed for use on
resource-constrained systems: from simple embedded environmental sensors and LED
wearables to sophisticated smart watches and IoT wireless gateways.

The Zephyr kernel supports multiple architectures, including ARM Cortex-M, Intel
x86, ARC, NIOS II, Tensilica Xtensa, and RISC-V. The full list of supported
boards can be found :ref:`here <boards>`.

Licensing
*********

Zephyr is permissively licensed using the `Apache 2.0 license`_
(as found in the ``LICENSE`` file in the
project's `GitHub repo`_).  There are some
imported or reused components of the Zephyr project that use other licensing,
as described in :ref:`Zephyr_Licensing`.

.. _Apache 2.0 license:
   https://github.com/zephyrproject-rtos/zephyr/blob/master/LICENSE

.. _GitHub repo: https://github.com/zephyrproject-rtos/zephyr


Distinguishing Features
***********************

The Zephyr kernel offers a number of features that distinguish it from other
small-footprint OSes:

**Single address-space**
   Combines application-specific code
   with a custom kernel to create a monolithic image that gets loaded
   and executed on a system's hardware. Both the application code and
   kernel code execute in a single shared address space.

**Highly configurable / Modular for flexibility**
   Allows an application to incorporate *only*
   the capabilities it needs as it needs them, and to specify their
   quantity and size.

**Cross Architecture**
   Supports a wide variety of :ref:`supported boards<boards>` with different CPU
   architectures and developer tools. Contributions have added support
   for an increasing number of SoCs, platforms, and drivers.

**Compile-time resource definition**
   Allows system resources
   to be defined at compile-time, which reduces code size and
   increases performance for resource-limited systems.

**Minimal and Configurable error checking**
   Provides minimal runtime error checking
   to reduce code size and increase performance. An optional error-checking
   infrastructure is provided to assist in debugging during application
   development.

**Memory Protection**
   Implements configurable architecture-specific stack-overflow protection,
   kernel object and device driver permission tracking, and thread isolation
   with thread-level memory protection on x86, ARC, and ARM architectures,
   userspace, and memory domains.

**Native Networking Stack supporting multiple protocols**
   Networking support is fully featured and optimized, including LwM2M
   and BSD sockets compatible support.  Bluetooth Low Energy 5.0 support
   includes BLE Mesh and a Bluetooth qualification-ready BLE controller.
   OpenThread support (on Nordic chipsets) is also provided - a mesh
   network designed to securely and reliably connect hundreds of products
   around the home.

**Native Linux, macOS, and Windows Development**
   A command-line CMake build environment runs on popular developer OS
   systems. A native POSIX port, lets you build and run Zephyr as a native
   application on Linux and other OSes, aiding development and testing.

**Extensive suite of services**
   Zephyr offers a number of familiar services for development:

   * *Multi-threading Services* for cooperative, priority-based,
     non-preemptive, and preemptive threads with optional round robin
     time-slicing.  Includes pthreads compatible API support.

   * *Interrupt Services* for compile-time registration of interrupt handlers.

   * *Memory Allocation Services* for dynamic allocation and freeing of
     fixed-size or variable-size memory blocks.

   * *Inter-thread Synchronization Services* for binary semaphores,
     counting semaphores, and mutex semaphores.

   * *Inter-thread Data Passing Services* for basic message queues, enhanced
     message queues, and byte streams.

   * *Power Management Services* such as tickless idle and an advanced idling
     infrastructure.

   * *File system* with Newtron Flash Filesystem (NFFS) and FATFS
     support, FCB (Flash Circular Buffer) for memory constrained
     applications, and file system enhancements for logging and
     configuration.

   * *ZTest infrastructure* and a coverage test suite for continuous
     integration testing and verification as features are added and updated.


.. include:: ../../README.rst
   :start-after: start_include_here


Fundamental Terms and Concepts
******************************

See :ref:`glossary`
