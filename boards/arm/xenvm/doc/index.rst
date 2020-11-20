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
* GICv2
* SBSA (subset of PL011) UART controller


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
| SBSA UART    | emulated    | serial port          |
+--------------+-------------+----------------------+
| ARM TIMER    | virtualized | system clock         |
+--------------+-------------+----------------------+

The kernel currently does not support other hardware features on this platform.

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

Serial Port
-----------

This board configuration uses a single serial communication channel using SBSA
UART. This is a minimal UART implementation provided by Xen. Xen PV Console is
not supported at this moment.

Interrupt Controller
--------------------

By default, GICv2 is selected. If your hardware is based on GICv3, you can
configure Zephyr to use it, by amending device tree and Kconfig
option in "xenvm" SoC as well as guest configuration file.

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

No Xen-specific features are supported at the moment. This includes:

* Xen Enlighten memory page
* XenBus
* Xen event channels
* Xen grant tables
* Xen PV drivers (including PV console)

Building and Running
********************

Use this configuration to run basic Zephyr applications and kernel tests as Xen
guest, for example, with the :ref:`synchronization_sample`:


.. code-block::

   $ west build -b xenvm samples/synchronization

This will build an image with the synchronization sample app. Next, you need to
create guest configuration file :code:`zephyr.conf`. There is example:

.. code-block::

   kernel="zephyr.bin"
   name="zephyr"
   vcpus=1
   memory=16
   gic_version="v2"
   on_crash="preserve"
   vuart="sbsa_uart"

You need to upload both :code:`zephyr.bin` and :code:`zephyr.conf` to your Dom0
and then you can run Zephyr by issuing

.. code-block::

   $ xl create zephyr.conf

Next you need to attach to SBSA virtual console:

.. code-block::

   $ xl console -t vuart zephyr

You will see Zephyr output:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.4.0-1137-g5803ee1e8183  ***
   thread_a: Hello World from cpu 0 on xenvm!
   thread_b: Hello World from cpu 0 on xenvm!
   thread_a: Hello World from cpu 0 on xenvm!
   thread_b: Hello World from cpu 0 on xenvm!
   thread_a: Hello World from cpu 0 on xenvm!

Exit xen virtual console by pressing :kbd:`CTRL+[`

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
