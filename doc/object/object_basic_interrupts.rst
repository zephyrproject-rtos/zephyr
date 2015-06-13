Interrupt Service Routines
##########################


General Information
*******************

Interrupt Service Routines are execution threads that run in response to
a hardware or software interrupt. They will preempt the execution of
any task or fiber running at the time the interrupt occurs.
Consequently, ISRs react fastest to hardware. Routines in the
nanokernel wake up with a very low overhead.

.. warning::

   ISRs prevent other parts of the system from running. Therefore,
   all code in these routines should be confined to short, simple
   routines.

.. todo:: Insert how an ISR can be installed both static and dynamic.


Both dynamic and static ISRs can be installed. See
`Installing a Dynamic ISR`_ and `Installing a Static ISR`_ for more
details. An ISR cannot be installed in the project file, only a task or
a driver initialization call can install an ISR.

When an ISR wakes up a fiber, there is only one context switch directly
to the fiber. When an ISR wakes up a task, there is first a context
switch to the nanokernel, and then another context switch to the task
in the microkernel.


Interrupt Stubs
***************

Interrupts stubs are small pieces of assembler code that connect your
ISR to the Interrupt Descriptor Table (IDT). The interrupt stub informs
the kernel when an interrupt is in progress, performs interrupt
controller specific work, invokes your ISR and informs the kernel when
the interrupt processing is complete. The stub address is registered in
the Interrupt Descriptor Table. The stub references your ISR and the
stubs can either be generated dynamically or statically.


Interrupt Service Routine APIs
******************************

The table lists the ISR Application Program Interfaces. There are a
number of calls that an ISR can use to switch between different
processing levels.

.. note::

   Application Program Interfaces of the ISRs are architecture-
   specific because they are implemented in the interrupt controller
   device driver for that processor or board. The architecture specific
   implementation can be found in the corresponding documentation for
   each architecture.

+-------------------------+---------------------------+
| Call                    | Description               |
+=========================+===========================+
| :c:func:`irq_enable()`  | Enables a specific IRQ.   |
+-------------------------+---------------------------+
| :c:func:`irq_disable()` | Disables a specific IRQ.  |
+-------------------------+---------------------------+
| :c:func:`irq_lock()`    | Locks out all interrupts. |
+-------------------------+---------------------------+
| :c:func:`irq_unlock()`  | Unlocks all interrupts.   |
+-------------------------+---------------------------+


Installing a Dynamic ISR
************************

Use :c:func:`irq_connect()` to install and connect an ISR stub
dynamically. :c:func:`irq_connect()` is processor-specific. There is no
API method to uninstall a dynamic ISR.


Installing a Static ISR
***********************

The contents of a static interrupt stub are complex and board specific.
They are generally created manually as part of the BSP. A stub is
installed statically into the Interrupt Descriptor Table using one of
the macros detailed in following table. The table lists the macros you
can use to identify and register your static ISRs into the Interrupt
Descriptor Table. The IA-32 interrupt descriptor allows for the setting
of the privilege level, DPL, at which the interrupt can be triggered.
The Zephyr OS assumes all device drivers are kernel mode (ring 0) as
opposed to user-mode (ring 3). Therefore, these macros always set the
DPL to 0.

The IDT Macros
==============


+--------------------------+-------------------------------------------------------------------------+
| Call                     | Description                                                             |
+==========================+=========================================================================+
| NANO_CPU_INT_REGISTER( ) | Use this macro to register a driver's                                   |
|                          | interrupt                                                               |
|                          | handler statically when the vector number is known at compile time.     |
+--------------------------+-------------------------------------------------------------------------+
| SYS_INT_REGISTER( )      | Use this macro to register a driver's                                   |
|                          | interrupt handler statically when                                       |
|                          | the vector number is not known at compile time but the priority and IRQ |
|                          | line are. The BSP is responsible for implementing this macro in board.h |
|                          | to generate a vector from the priority and IRQ line at compile time.    |
|                          | The macro is intended to provide a level of abstraction between the BSP |
|                          | and the driver.                                                         |
+--------------------------+-------------------------------------------------------------------------+


Interrupt Descriptor Table
**************************

The Interrupt Descriptor Table (IDT) is a data structure that implements
an interrupt vector table used by the processor to determine the
correct response to interrupts and exceptions. To optimize boot
performance and increase security, the kernel implements targets
using a statically created Interrupt Descriptor Table, interrupt stubs
and exception stubs. A static Interrupt Descriptor Table improves boot
performance because:

* No CPU cycles are used to construct the Interrupt Descriptor Table
  at boot up.

* No CPU cycles are used to create interrupt stubs at boot up.

* No CPU cycles are used to create exceptions stubs at run-time.

The statically created Interrupt Descriptor Table can still be updated
at run-time despite being write-protected. There may be instances where
updating the Interrupt Descriptor Table at run-time is required, for
example, in order to install dynamic interrupts. The decision of
whether a target implements dynamic or static interrupts is determined
at compile time automatically based on the configuration.


Securing the Interrupt Descriptor Table
***************************************

Typically the IDT resides in the data section. Enable the Section Write
Protection feature to move the Interrupt Descriptor Table to the rodata
section and to mark all pages of memory in which the Interrupt
Descriptor Table resides as read-only. Enabling the Section Write
Protection feature places dynamic interrupt stubs into the text section
protecting them. A system where execute in place, XIP, support is
enabled, assumes the text section and read-only data section reside in
read-only memory, such as flash memory or ROM. In this scenario dynamic
interrupt stubs are not possible. The Interrupt Descriptor Table cannot
be updated at runtime. Therefore enabling the Section Write Protection
feature blocks generating dynamic interrupt stubs and updating the
Interrupt Descriptor Table at runtime.

Note This implementation of XIP does not support a ROM-resident
Interrupt Descriptor Table. When the segmentation feature is enabled,
execution of code in the data segment is not allowed. If the
segmentation feature is enabled and section write protection is not
enabled, dynamic interrupt stubs move to the text section, but they are
still writable.

The following is an example of a dynamic interrupt stub for x86:

.. code-block:: c

   static NANO_CPU_INT_STUB_DECL (deviceStub);

   void deviceDriver (void)

   {

   .
   .
   .

   nanoCpuIntConnect (deviceIRQ, devicePrio, deviceIntHandler,
   deviceStub);

   .
   .
   .

   }

This feature is part of the enhanced security profile in Zephyr OS.


Working with ISRs
*****************


Triggering Interrupts
=====================

The processor starts up an ISR when a hardware interrupt is received.
When one of the interrupt pins of the processor core is triggered, the
processor jumps to the appropriate interrupt routine. To interface this
hardware event with software, the kernel allows you to attach an ISR
to the interrupt signal.

An ISR can interface with a fiber using the nanokernel Application
Program Interfaces. The ISR can wake up a task using the microkernel
synchronization objects, an event or invoking the event handler. The
nanokernel affords them the lowest startup overhead because ISRs are
triggered from the hardware level. No context switch is needed to start
up an ISR.

When an interrupt occurs, all fibers and all tasks wait until the
interrupt is handled. If an application is executing a task or a fiber
is running, it is interrupted until the ISR finishes.

An ISR implementation is typically very hardware-specific because it
interfaces directly with a hardware interrupt and starts to run because
of it. The details of how this happens are described in your processors
documentation.

Prototype your hardware-specific functionality in a task, before you
move it to the ISR code.

If an ISR calls a channel service with a signal action, any fiber
rescheduling resulting from this call is delayed until all interrupt
handlers terminate. Therefore, use only the nano_Isr Application
Program Interfaces, as these do not invoke the system kernel scheduler
for a signal action. Keep in mind that there is no need for a swap at
this point; the caller has the highest priority already. Once the last
stacked interrupt terminates, the nanokernel scheduler must be called
to verify if a swap from the task to a fiber is necessary.

An ISR must never call any blocking channel Application Program
Interface. It would block the current fiber and all other interrupt
handlers that are stacked below the ISR.

The kernel supports interrupt nesting. When an ISR is running, it can
be interrupted when a new interrupt is received.


Using Interrupt Service Routines
================================

If interrupts come in at high speed, parts of your code can be at the
ISR level. If code is at the interrupt level, it avoids a context
switch making it faster. If interrupts come in at low speeds, the ISR
should only wake up a fiber or a task. That fiber or task should do all
the processing, not the ISR, even if the task can be interrupted by
fibers and ISRs. Keep fibers and ISRs short to ensure predictability.

For example, take an application that implements an algorithm in an ISR.
Suppose the algorithm takes one second to finish calculating. The
application has a task in the background that interfaces with a host
machine to plot data on the screen. The task updates the screen image
five times per second to provide a smooth screen display. This
application as a whole does not behave predictably if an interrupt is
received. The ISR starts calculating for one second and causes an
unexpected delay. The same holds true if the algorithm is implemented
using a fiber. The user sees an interleaved screen output. This example
is extreme but it shows that short fibers and short ISRs make the
system more predictable.


Implementing Interrupt Service Routines
***************************************

Most processors require that ISRs be coded in assembler. To make the
implementation easier, several assembler macros are available to do the
most common jobs. Because the ISRs block all other processing, always
implement the actual handling of the interrupt in a fiber or a task.
Where to handle the interrupt is a design choice that must be made
while considering the performance of the processor and the frequency of
the interrupt.


Coordinating ISRs and Events
****************************

An ISR can send a signal from the nanokernel to the microkernel to
trigger an event. Your setup can work with an event handler, or without
one. If there is no event handler and your task is waiting for the
event, the ISR wakes up the task when it triggers the event. If you
have an event handler, the ISR triggers the event handler routine. This
event handler then determines if the task wakes up or not.

.. warning::

   Implement or process a buffer in an event handler if you input
   comes in at a high speed.

Command Packet Sets
*******************

A command packet set is a group of statically-allocated command packets.
A command packet is accessible to any application running in kernel
space. They are necessary when signaling a semaphore from an ISR via
:c:func:`Isr_sem_give()` since command packets are processed after the
ISR finishes. That makes stack-allocated command packets unsafe for
this purpose. A statically-allocated command packet is implicitly
released after being processed. Consequently, the operating system does
not track the use-status of any statically-allocated command packet.

There is a small but unavoidable risk of a command packet's processing
being incomplete before the ISR runs again and tries to reuse the
packet. To further minimize this risk the kernel introduces command
packet sets. Fundamentally, a command packet set is a simple ring
buffer. Retrieve command packets from the set using
:c:func:`cmdPktGet()`. Each command packet has to be processed in a
near-FIFO since no use-status checking is performed a packet is
retrieved. In order to minimize the risk of packet corruption from
premature reuse, drivers that have an ISR component should use their
own command packet set and not use a common set for many drivers.
Create a command packet set in global memory using:

.. code-block:: c

   CMD_PKT_SET_INSTANCE(setVariableName, #ofCommandPacketsInSet);


Task Level Interrupt Processing
*******************************

The task level interrupt processing feature permits to service
interrupts at the task level, without having to develop kernel level
ISRs. The *MAX_NUM_TASK_DEVS* kernel configuration option specifies the
total number of devices needing task-level interrupt support.

The default setting of 0 disables the following interfaces:
:c:func:`task_irq_alloc()`, :c:func:`task_irq_free()`,
:c:func:`task_irq_ack()` and :c:func:`task_irq_test()`. Each device has
a well-known identifier in the range from 0 to *MAX_NUM_TASK_DEVS*-1.

The Zephyr OS allows kernel tasks to bind to devices at run-time by
calling :c:func:`task_irq_alloc()`. A task may bind itself to multiple
devices by calling this routine multiple times but a given device can
be bound to only a single task at any point in time. The registering
task specifies the device it wishes to use, the associated IRQ and
priority level for the device's interrupt. It gets the assigned
interrupt vector in return. The interrupt associated with the device is
enabled once the task has registered to use a device. Whenever the
device generates an interrupt, the kernel automatically runs an ISR
that disables the interrupt and records its occurrence.

The task associated with the device can use :c:func:`taskDevIntTest()`
to determine if the device's interrupt has occurred. Alternatively, it
can use :c:func:`task_irq_test_wait()` or
:c:func:`task_irq_test_wait_timeout()` to wait until an interrupt is
detected.

After the task took the appropriate action to service an interrupt
generated by the device, it calls :c:func:`task_irq_ack()` to re-enable
the device's interrupt. The task can call :c:func:`task_irq_free()` to
unbind itself from a device that it no longer wishes to use. If the
registered device needs change its priority level, it must first
unregister and then register again with the new priority. To provide
security against device misuse, a device should only be tested,
acknowledged, and deregistered by a task if that task registered the
device. Restrict which task can register a given device or use the
device after registration, at the shim layer.
