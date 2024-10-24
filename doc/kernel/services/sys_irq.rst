.. _sys_irq:

SYS IRQ (SYStem Interrupt ReQuest) WIP
######################################

The SYS IRQ kernel service utilizes the devicetree and device driver model to
provide interrupt handling scalably, portably and efficiently.

Design
******

The design provides clear separation between interrupt controllers, interrupt
generating devices, and interrupt request management.

IRQ (Interrupt ReQuests)
========================

An IRQ is a hardware event which, when triggered, invokes a handler in
software.

INTC (INTerrupt Controller)
===========================

INTCs invoke SYS IRQ INTL (INTerrupt Line) handlers when an interrupt is detected
on the corresponding INTL.

It is the responsibility of the INTC device driver, SoC and Arch, to do any and
all preparation required to invoke the SYS IRQ INTL handlers from a execution
context.

Any and all configuration of INTLs is performed by SYS IRQ through the INTC device
driver API.

INTD (INTerrupt generating Device)
==================================

INTDs route IRQs through INTLs. Its' IRQs are defined in the devicetree, and managed
by SYS IRQ.

INTD device drivers define their IRQ handlers which SYS IRQ will invoke when the
IRQ is triggered.

INTD device drivers configure and enable/disable their IRQs using SYS IRQ.

Porting guide
*************

The following section describes how to port a series SoCs, Archs and device drivers
from legacy IRQ to SYS IRQ.

INTC
====

Device drivers implementing the INTC API for all INTCs must be written for each
INTC.

INTD
====
