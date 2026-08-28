:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.6:

Migration guide to Zephyr v4.6.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.5.0 to
Zephyr v4.6.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.6>`.

.. contents::
    :local:
    :depth: 2

Common
******

Build System
************

Kernel
******

Boards
******

Device Drivers and Devicetree
*****************************

.. Only place contents common to all device drivers here. Contents specific to one driver subsystem
   goes into its own subsection, below.

.. Group contents in this section by subsystem, e.g.:
..
.. ADC
.. ===
..
.. ...

.. zephyr-keep-sorted-start re(^\w) ignorecase

Interrupt Controller
====================

* Espressif SoCs describe the interrupt matrix with the multi-level interrupt model
  (:kconfig:option:`CONFIG_MULTI_LEVEL_INTERRUPTS`). Out-of-tree devicetree nodes and drivers
  need updating:

  * A consumer that shares a CPU interrupt line points at that line's
    :dtcompatible:`espressif,esp32-cpu-interrupt` node (``cpu_interrupt_<n>``) and names only
    its interrupt matrix source, for example ``interrupt-parent = <&cpu_interrupt_5>;
    interrupts = <UART0_INTR_SOURCE>;``. The old ``<source priority flags>`` cells on
    :dtcompatible:`espressif,esp32-intc` are gone.
  * ``ESP_INTR_FLAG_IRAM`` keeps a CPU line running while the flash cache is off; every other line
    is masked for that time. A shared line gets it in ``ESP_CPU_INTERRUPT(n, flags)``, a dedicated
    line in the flags cell of its consumer. The flag passed to :c:macro:`IRQ_CONNECT` is not used.
  * A consumer that has a CPU line to itself points at :dtcompatible:`espressif,esp32-intc` and
    names ``<line source flags>``.
  * Peripherals whose status bits share one source point at an
    :dtcompatible:`espressif,esp32-l3-intc` node and name an index into its ``bits`` list.
  * The ``esp_intr_alloc()`` family of functions and ``ESP_PRIO_TO_FLAGS()`` /
    ``ESP_INT_FLAGS_CHECK()`` are removed. Drivers connect their interrupt with
    :c:macro:`IRQ_CONNECT` (or :c:func:`irq_connect_dynamic`) on the devicetree IRQ number,
    e.g. ``DT_INST_IRQN(n)``, and enable it with :c:func:`irq_enable`.
  * The interrupt-controller headers are renamed to
    ``zephyr/dt-bindings/interrupt-controller/espressif-<soc>-intmux.h``.

.. zephyr-keep-sorted-stop

Bluetooth
*********

Networking
**********

Other subsystems
****************

Modules
*******

Architectures
*************
