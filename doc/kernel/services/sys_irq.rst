.. _sys_irq:

System interrupt request management (SYS IRQ)
#############################################

The SYS IRQ kernel service is an implementation of the the interrupt tree model specified in the
devicetree specification, based on zephyr's devicetree and device driver model.

Notable features are:

* Scaling to any number of root interrupt controllers, with any number of nested/child interrupt
  controllers, which can also be described as any level of, and number of,
  :ref:`Multi-level interrupts <multi_level_interrupts>`.
* Low coupling between hardware architecture and interrupt request management.
* Relies on C code generated from the devicetree for high compiler portability and optimizability,
  negating the need for post processing and multi stage building.

Design
******

The design seperates interrupt management into the following components:

Interrupt controller (INTC)
===========================

Interrupt controllers are devices which manage interrupt lines. They implement the
:ref:`INTC device driver API <intc_interface>` to configure interrupt lines, and are
responsible for calling the kernels interrupt line handlers upon an interrupt occuring.

Interrupt generating device (INTD)
==================================

Interrupt generating devices are connected to one or more interrupt lines. They use the
:ref:`INTC device driver API <intc_interface>` to configure their interrupt lines, and
implement interrupt request handlers for their interrupt lines which are called by the kernel.

System interrupt request management (SYS IRQ)
=============================================

System interrupt request management exposes the interrupt line handlers the interrupt controllers
to call into, and manages the connection between the interrupt line handlers and the interrupt
request handlers registered by the interrupt generating devices.
