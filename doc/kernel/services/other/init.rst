.. _sys_init_api:

System Initialization
#####################

Before the application's ``main()`` is entered, Zephyr runs a sequence of
initialization functions registered by the kernel, by device drivers and by
subsystems. Each function is registered with :c:macro:`SYS_INIT`, with
:c:macro:`DEVICE_DT_DEFINE` (see :ref:`device_model_api`) or with one of their
variants, and is assigned an *initialization level* that selects the phase of
the boot sequence it runs in.

Boot Sequence
*************

The boot sequence runs the initialization levels in the order shown below.
The levels themselves are described in `Initialization Levels`_.

.. mermaid::
   :caption: The boot sequence and the initialization levels
   :alt: Flowchart of the boot sequence. From z_cstart the system runs the
       EARLY level, architecture initialization, the SoC and board early
       hooks, the PRE_KERNEL level, the deprecated PRE_KERNEL_2 band, kernel
       start with the switch to the main thread, the POST_KERNEL level, the
       SoC and board late hooks, the APPLICATION level, creation of the static
       threads and start of the secondary CPUs, the PRE_MAIN level, and
       finally main.

   flowchart TD
       start(("z_cstart"))
       early[["EARLY"]]
       arch("arch_kernel_init")
       ehooks("soc_early_init_hook<br/>board_early_init_hook")
       pk[["PRE_KERNEL"]]
       pk2[["PRE_KERNEL_2<br/>deprecated"]]
       kern("kernel start<br/>switch to main thread")
       postk[["POST_KERNEL"]]
       lhooks("soc_late_init_hook<br/>board_late_init_hook")
       app[["APPLICATION"]]
       threads("static threads created<br/>secondary CPUs started, on SMP")
       premain[["PRE_MAIN"]]
       done(("main"))

       start --> early --> arch --> ehooks --> pk --> pk2 --> kern
       kern --> postk --> lhooks --> app --> threads --> premain --> done

Besides the levels, the sequence includes architecture initialization, the SoC
and board hooks (see :ref:`soc_porting_guide` and :ref:`board_porting_guide`),
the creation of the static threads defined with :c:macro:`K_THREAD_DEFINE`
and, on SMP systems, the start of the secondary CPUs (see :ref:`smp_arch`).
They are shown so that it is clear what each level can rely on having happened
already.

``EARLY``, ``PRE_KERNEL`` and the deprecated ``PRE_KERNEL_2`` band run before
the kernel is able to schedule, ``PRE_KERNEL`` on the interrupt stack;
everything from ``POST_KERNEL`` on runs in the context of the main thread
(with :kconfig:option:`CONFIG_MULTITHREADING`). Code can check whether the
system is still in a pre-kernel phase with :c:func:`k_is_pre_kernel`.

Initialization Levels
*********************

``EARLY``
   Runs immediately after entering the C domain, before any architecture
   initialization. Reserved for architecture and SoC code that implements or
   extends architecture support and depends on a driver or system service
   being available that early.

``PRE_KERNEL``
   Runs in the kernel's initialization context, on the interrupt stack. Kernel
   services are not available yet, but the interrupt subsystem is configured,
   so interrupts may be set up. This is the level for devices that depend only
   on hardware present in the SoC, or on other ``PRE_KERNEL`` devices.

``POST_KERNEL``
   Runs once the kernel is alive, in the context of the main thread. Kernel
   primitives (semaphores, work queues, timers, ...) may be used, so this is
   the level for devices whose initialization needs kernel services.

``APPLICATION``
   Runs after ``POST_KERNEL``, before the static threads are created and,
   on SMP systems, before the secondary CPUs are started. Intended for
   application-level services.

``PRE_MAIN``
   Runs as the last step before ``main()`` is entered. At this point the
   static threads exist and, on SMP systems, every secondary CPU started at
   boot is online -- bringing up a secondary CPU can be deferred to run time,
   in which case it is not. Only :c:macro:`SYS_INIT` entries may use this
   level.

Devices may only be defined at ``PRE_KERNEL`` or ``POST_KERNEL``;
:c:macro:`SYS_INIT` entries may use any level.

The following level tokens are deprecated. They keep working so that existing
code and its ordering are unaffected, but new code must not use them:

``PRE_KERNEL_1``
   Alias of ``PRE_KERNEL``. Its entries share the ``PRE_KERNEL`` level and are
   ordered together with them, by priority. Use ``PRE_KERNEL``.

``PRE_KERNEL_2``
   Runs in the same execution context as ``PRE_KERNEL``, but all of its
   entries run after every ``PRE_KERNEL`` entry. It exists only as a coarse
   "after everything else in ``PRE_KERNEL``" barrier. Migrate to
   ``PRE_KERNEL``, expressing the actual dependency with one of the mechanisms
   described below.

``SMP``
   Alias of ``PRE_MAIN``, which runs at the same point in the boot sequence
   but is not conditioned on :kconfig:option:`CONFIG_SMP`. Use ``PRE_MAIN``.

Ordering Within a Level
***********************

The order in which the entries of one level run is decided entirely at build
time: the linker sorts the init entries by the sort key encoded in their
section name, and the boot code simply walks the resulting array. There is no
runtime dependency resolution and no runtime cost for any of the mechanisms
below.

Every entry is ordered either by a hand-picked numeric priority or
automatically, and within a level all numeric-priority entries run before any
automatically ordered one:

.. mermaid::
   :caption: Order of the entries of a single initialization level
   :alt: Within one level, entries with a numeric priority run first in
       increasing priority order from 0 to 999, followed by the automatically
       ordered entries: first the ones ordered by their devicetree
       dependencies, then the anchored ones.

   flowchart LR
       subgraph numeric["numeric priority"]
           direction LR
           p0["0"] --> pdots["..."] --> p999["999"]
       end
       subgraph auto["automatic"]
           direction LR
           dt["devicetree order"] --> anchors["anchors"]
       end
       p999 --> dt

Numeric priorities
==================

An entry registered with :c:macro:`SYS_INIT` or :c:macro:`DEVICE_DT_DEFINE`
carries a priority in the range 0 to 999; lower values run earlier. The
priority must be a decimal integer literal without leading zeroes or sign
(e.g. ``32``), or an equivalent symbolic name (e.g.
``#define MY_INIT_PRIO 32``). Symbolic *expressions* are **not** permitted
(e.g. ``CONFIG_KERNEL_INIT_PRIORITY_DEFAULT + 5``).

.. code-block:: c

   SYS_INIT(my_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

Priorities are a blunt tool: they express *when* an entry runs, not *what* it
depends on. Two entries that depend on each other but are given the same
priority are ordered by link order, which is not guaranteed to be stable, and
an entry whose priority was chosen to clear some other entry's has no way of
noticing when that other entry moves.

.. note::

   Numeric priorities are discouraged in new code and are expected to be
   deprecated in a future release. Prefer declaring what an entry depends on,
   with one of the mechanisms described below, over picking a number.

Ordering a device by its devicetree dependencies
================================================

A device defined with :c:macro:`DEVICE_DT_DEFINE_AUTO` takes no priority
argument. It is ordered by its devicetree dependency ordinal, so it
automatically initializes after the devices its node depends on, such as the
bus it hangs off or its interrupt parent:

.. code-block:: c

   DEVICE_DT_INST_DEFINE_AUTO(0, my_init, NULL, &data, &config,
                              POST_KERNEL, &my_api);

Because the ordering follows the devicetree, adding a dependency to the node
is enough; no priority has to be revisited. Bus-specific wrappers exist for
the common cases, for example ``I2C_DEVICE_DT_DEFINE_AUTO()`` and
``SPI_DEVICE_DT_DEFINE_AUTO()``.

Running after a specific device
===============================

Code that consumes a device, but is not a device itself, can be ordered
directly after that device with :c:macro:`SYS_INIT_DEPENDS`, which takes the
devicetree node of the device instead of a priority:

.. code-block:: c

   SYS_INIT_DEPENDS(my_init, PRE_KERNEL, DT_CHOSEN(zephyr_console));

This is the natural replacement for a hand-picked priority chosen only to be
larger than some driver's priority, and for a ``PRE_KERNEL_2`` registration
used to run "after the drivers".

Running after another service
=============================

Services that are not devices, and therefore have no devicetree node to be
ordered against, can order themselves with named *anchors*. A service
publishes an anchor key with :c:macro:`SYS_ANCHOR`, conventionally in its
header next to its API, and registers its init function with
:c:macro:`SYS_INIT_ANCHORED`:

.. code-block:: c

   /* my_service.h */
   #define SYS_ANCHOR_my_service SYS_ANCHOR(my_service)

   /* my_service.c */
   SYS_INIT_ANCHORED(my_service, my_service_init, POST_KERNEL);

Another service orders itself after it by extending that key with
:c:macro:`SYS_ANCHOR_AFTER`. Depending on a service therefore means including
its header, and a missing or misspelled dependency is a compile error:

.. code-block:: c

   #include <my_service.h>

   #define SYS_ANCHOR_my_client SYS_ANCHOR_AFTER(SYS_ANCHOR_my_service, my_client)
   SYS_INIT_ANCHORED(my_client, my_client_init, POST_KERNEL);

.. note::

   The dependency is passed as the *anchor key macro* of the service depended
   on (``SYS_ANCHOR_my_service``), not as its name. This is required for
   dependency chains of arbitrary depth to expand correctly.

When the service depended on is itself optional, use
:c:macro:`SYS_ANCHOR_AFTER_IF` so that the dependent still initializes, just
unordered, in configurations where the dependency does not exist:

.. code-block:: c

   #define SYS_ANCHOR_my_client \
           SYS_ANCHOR_AFTER_IF(CONFIG_MY_SERVICE, SYS_ANCHOR_my_service, my_client)

A service that declares no dependency runs at the end of its level, after
every numeric-priority and devicetree-ordered entry. This is the idiomatic
replacement for a ``PRE_KERNEL_2`` entry used purely as an end-of-level
barrier.

An anchor key encodes one chain of dependencies. A service that depends on
several others extends the key of the one initialized last; the remaining
dependencies are checked at build time (see below).

The mechanisms compose: a chain may start at a device ordered by the
devicetree, continue with a service ordered after that device, and end with a
service anchored after that service.

.. mermaid::
   :caption: A dependency chain mixing the three mechanisms
   :alt: The i2c0 bus device is initialized first, then a sensor device
       defined with DEVICE_DT_DEFINE_AUTO on a node under that bus, then a
       service registered with SYS_INIT_DEPENDS on the sensor node, then a
       second service anchored after the first one with SYS_ANCHOR_AFTER.

   flowchart LR
       bus["i2c0<br/>bus device"]
       sensor["sensor<br/>DEVICE_DT_DEFINE_AUTO"]
       svc["my_service<br/>SYS_INIT_DEPENDS"]
       client["my_client<br/>SYS_INIT_ANCHORED"]

       bus -->|devicetree parent| sensor
       sensor -->|node id| svc
       svc -->|anchor key| client

Choosing a mechanism
====================

+-------------------------------------------+------------------------------------------+
| The entry ...                             | Use                                      |
+===========================================+==========================================+
| is a device with devicetree dependencies  | :c:macro:`DEVICE_DT_DEFINE_AUTO`         |
+-------------------------------------------+------------------------------------------+
| must run after a specific device          | :c:macro:`SYS_INIT_DEPENDS`              |
+-------------------------------------------+------------------------------------------+
| must run after another service            | :c:macro:`SYS_INIT_ANCHORED` with        |
|                                           | :c:macro:`SYS_ANCHOR_AFTER`              |
+-------------------------------------------+------------------------------------------+
| must run after everything else in a level | :c:macro:`SYS_INIT_ANCHORED` with        |
|                                           | :c:macro:`SYS_ANCHOR`                    |
+-------------------------------------------+------------------------------------------+
| has no ordering requirement               | :c:macro:`SYS_INIT` with a priority      |
+-------------------------------------------+------------------------------------------+

Inspecting and Validating the Sequence
**************************************

To see the final sequence of initialization calls as produced by the linker,
use the ``initlevels`` CMake target, for example ``west build -t initlevels``.

When :kconfig:option:`CONFIG_CHECK_INIT_PRIORITIES` is enabled (the default),
the build also validates the sequence and fails on ordering errors:

* a device that is initialized before a device it depends on in the
  devicetree, or that shares its priority;
* an anchored entry whose dependency is not linked into the image, or which
  runs at an earlier level than that dependency.

Because the checks read the linked image, they validate the order that will
actually be executed, whatever mechanism produced it.

Deferred Initialization
***********************

A device can also be left uninitialized at boot and brought up later by the
application with :c:func:`device_init`, by adding the ``zephyr,deferred-init``
property to its devicetree node. See :ref:`device_model_api`.

API Reference
*************

.. doxygengroup:: sys_init
