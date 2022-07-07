.. xenvm:

ARMv8 Xen Virtual Machine Example
#################################

Overview
********

This board allows to run Zephyr as Xen guest on any ARMv8 board that supports
ARM Virtualization Extensions. This is example configuration, as almost any VM
configuration is unique in many aspects.

.. figure:: xen_project_logo.png
   :align: center
   :alt: XenVM

   Xen virtual Guest (Credit: Xen Project)

It provides minimal set of devices:

* ARM Generic timer
* GICv2/GICv3

Hardware
********
Supported Features
==================

The following hardware features are supported:

+--------------+-------------+----------------------+
| Interface    | Controller  | Driver/Component     |
+==============+=============+======================+
| GIC          | virtualized | interrupt controller |
+--------------+-------------+----------------------+
| ARM TIMER    | virtualized | system clock         |
+--------------+-------------+----------------------+

The kernel currently does not support other hardware features on this platform.

The default configuration using GICv2 can be found in the defconfig file:
    ``boards/arm64/xenvm/xenvm_defconfig``

The default configuration using GICv3 can be found in the defconfig file:
    ``boards/arm64/xenvm/xenvm_gicv3_defconfig``

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 8.32 MHz. This is the
default value, which should be corrected for user's actual hardware.

You can determine clock frequency of your ARM Generic Timer by inspecting Xen
boot log:

::

  (XEN) [    0.147541] Generic Timer IRQ: phys=30 hyp=26 virt=27 Freq: 8320 KHz

Interrupt Controller
--------------------

Depending on the version of the GIC on your hardware, you may choose one of the
following board configurations:

- ``xenvm_defconfig`` selects GICv2
- ``xenvm_gicv3_defconfig`` selects GICv3

CPU Core type
-------------

Default core in this configuration is Cortex A72. Depending on yours actual
hardware you might want to change this option in the same way as Interrupt
Controller configuration.

Known Problems or Limitations
==============================

Xen configures guests in runtime by providing device tree that describes guest
environment. On other hand, Zephyr uses static configuration that should be know
at build time. So there are chances, that Zephyr image created with default
configuration would not boot on your hardware. In this case you need to update
configuration by altering device tree and Kconfig options. This will be covered
in detail in next section.

Most of Xen-specific features are not supported at the moment. This includes:
* XenBus (under development)
* Xen grant tables (under development)
* Xen PV drivers

Now only following features are supported:
* Xen Enlighten memory page
* Xen event channels
* Xen PV console (2 versions: regular ring buffer based for DomU and consoleio for Dom0)
* Xen early console_io interface (mainly for debug purposes - requires debug version of Xen)

Building and Running
********************

Use this configuration to run basic Zephyr applications and kernel tests as Xen
guest, for example, with the :ref:`synchronization_sample`:

- if your hardware is based on GICv2:

.. code-block::

   $ west build -b xenvm samples/synchronization

- if your hardware is based on GICv3:

.. code-block::

   $ west build -b xenvm_gicv3 samples/synchronization

This will build an image with the synchronization sample app. Next, you need to
create guest configuration file :code:`zephyr.conf`. There is example:

.. code-block::

   kernel="zephyr.bin"
   name="zephyr"
   vcpus=1
   memory=16
   gic_version="v2"
   on_crash="preserve"

When using ``xenvm_gicv3`` configuration, you need to remove the ``gic_version``
parameter or set it to ``"v3"``.

You need to upload both :code:`zephyr.bin` and :code:`zephyr.conf` to your Dom0
and then you can run Zephyr by issuing

.. code-block::

   $ xl create zephyr.conf

Next you need to attach to PV console:

.. code-block::

   $ xl console zephyr

Also this can be performed via single command:

.. code-block::

   $ xl create -c zephyr.conf

You will see Zephyr output:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.4.0-1137-g5803ee1e8183  ***
   thread_a: Hello World from cpu 0 on xenvm!
   thread_b: Hello World from cpu 0 on xenvm!
   thread_a: Hello World from cpu 0 on xenvm!
   thread_b: Hello World from cpu 0 on xenvm!
   thread_a: Hello World from cpu 0 on xenvm!

Exit xen virtual console by pressing :kbd:`CTRL+]`

Updating configuration
**********************

As was said earlier, Xen describes hardware using device tree and expects that
guest will parse device tree in runtime. On other hand, Zephyr supports only
static, build time configuration. While provided configuration should work on
almost any ARMv8 host running in aarch64 mode, there is no guarantee, that Xen
will not change some values (like RAM base address) in the future.

Also, frequency of system timer is board specific and should be updated when running
Zephyr xenvm image on new hardware.

One can make Xen to dump generated DTB by using :code:`LIBXL_DEBUG_DUMP_DTB`
environment variable, like so:

.. code-block::

   $ LIBXL_DEBUG_DUMP_DTB=domu-libxl.dtb xl create zephyr.conf

Then, generated "domu-libxl.dtb" file can be de-compiled using "dtc" tool.

Use information from de-compiled DTB file to update all related entries in
provided "xenvm.dts" file. If memory layout is also changed, you may need to
update :code:`CONFIG_SRAM_BASE_ADDRESS` as well.

References
**********

`Xen ARM with Virtualization Extensions <https://wiki.xenproject.org/wiki/Xen_ARM_with_Virtualization_Extensions>`_

`xl.conf (guest configuration file) manual <https://xenbits.xen.org/docs/unstable/man/xl.cfg.5.html>`_
