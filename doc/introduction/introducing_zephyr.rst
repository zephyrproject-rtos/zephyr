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

**Minimal error checking**
   Provides minimal runtime error checking
   to reduce code size and increase performance. An optional error-checking
   infrastructure is provided to assist in debugging during application
   development.

**Extensive suite of services**
   Offers a number of familiar services for development:

   * *Multi-threading Services* for cooperative, priority-based,
     non-preemptive, and preemptive threads with optional round robin
     time-slicing.  Includes pthreads compatible API support.

   * *Interrupt Services* for compile-time registration of interrupt handlers.

   * *Memory Allocation Services* for dynamic allocation and freeing of
     fixed-size or variable-size memory blocks.

   * *Memory Protection* wtih architecture-specific stack-overflow protection,
     kernel object and device driver permission tracking, and thread isolation.

   * *Inter-thread Synchronization Services* for binary semaphores,
     counting semaphores, and mutex semaphores.

   * *Inter-thread Data Passing Services* for basic message queues, enhanced
     message queues, and byte streams.

   * *Power Management Services* such as tickless idle and an advanced idling
     infrastructure.

   * *Native Networking Stack* fully featured and optimized, including
     LwM2M and BSD sockets compatible support. OpenThread support is also
     provided - a mesh network designed to securely and reliably
     connect hundreds of products around the home.

   * *Bluetooth Low Energy* 5.0 support including a Bluetooth
     qualification-ready BLE controller, and BLE Mesh.

   * *File system* with Newtron Flash Filesystem (NFFS) and FAT
     support, and file system enhancements for logging and
     configuration.

   * *Native Linux, macOS, and Windows Development* with command-line
     Cmake build environment. A native POSIX port, letting you build and
     run Zephyr as a native application on Linux and other OSes aids
     development and testing.

   * *ZTest infrastructure* and a coverage test suite for continuous
     integration testing and verification as features are added and updated.


.. include:: ../../README.rst
   :start-after: start_include_here


Fundamental Terms and Concepts
******************************

See :ref:`glossary`
