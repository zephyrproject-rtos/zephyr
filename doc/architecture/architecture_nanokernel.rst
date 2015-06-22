.. _architecture_nanokernel:

The Nanokernel
##############

The Zephyr nanokernel is a minimal execution environment that offers a multi-context programming model without the overhead associated with other operating system kernels. It is ideal for single-core dedicated systems requiring a single background task. The nanokernel provides simple prioritized fibers and :abbr:`ISRs (Interrupt Service Routines)`. The nanokernel’s fast context switching gives it an advantage for systems where multiple asynchronous streams of data must be manipulated.

The nanokernel is intended for systems with a small amount of RAM, 1-50 KB;
simple buses, UART, I2C, SPI; and a single data processing task.
Typical applications of nanokernel only systems are embedded sensor hubs,
environmental sensors, simple LED wearables and store inventory tags.

Nanokernel Function
*******************

The nanokernel interfaces with the hardware providing support for multiple
ISRs, multiple execution fibers and a single processing task. The ISRs, fibers and task interact by means of the nanokernel objects.

The nanokernel manages the scheduling of fibers according to the activity of these objects, and provides the entry points necessary to integrate interrupt handles with the rest of the system. All communication inside the nanokernel is performed using the four channel objects: semaphores, FIFOs, LIFOs and stacks. 

The nanokernel is optimized for basic multitasking, synchronization, and data passing. Therefore, the APIs are streamlined for performance and the nanokernel provides significantly fewer APIs than the microkernel. 

The nanokernel has a low context switch latency. This is achieved by reducing the context switching overheads. A fiber runs until blocked because it is waiting for communication from another fiber or because it voluntarily yields. This cooperative context switch means that fibers are not preempted reducing the overhead.

Nanokernel Objects
******************
The most important objects of the nanokernel are:

* Semaphores
* Stacks
   * :abbr:`LIFO (Last In First Out)`s
   * :abbr:`FIFO (First In First Out)`s
* Timers

The following subsections contain general information about the nanokernel objects. For detailed information see: :ref:`nanokernelObjects`.

Nanokernel Semaphores
=====================

The nanokernel implements counting semaphores capable of tracking multiple
signals. One pending execution context and one or more signaling contexts
can signal a semaphore in an ISR, a fiber or a task. Similarly, one pending
execution context and one or more signaling contexts can attempt and fail to
take a semaphore in an ISR, a fiber or a task.

If a fiber pends on a semaphore, it will yield and be marked as not runnable.
On the other hand, a task pending on a semaphore will busy wait keeping the
CPU idle.

Nanokernel Stacks
=================

The nanokernel implements a temporary storage using stacks, LIFOs and FIFOs.
Execution contexts can push or pop one word of data from the stack. One
context pops and one or more contexts push data onto the stack.

In all execution contexts of the nanokernel can push data onto the stack.
Similarly, the nanokernel can attempt, without waiting, to pop of the stack
in all contexts. A fiber waiting on stack data will yield and be marked as
not runnable. A task waiting for data will busy wait keeping the CPU idle.

.. important::
   The nanokernel's stack implementation does not offer automatic stack
   overflow protection.

LIFOs
-----

LIFOs are specialized stacks where the last data that enters the stack is
also the first data to leave it. The nanokernel LIFOs can get and put data
of any size. The write operation allocates memory and reserves the first
word for the next pointer. LIFOs only one read operation at the time but
they allow multiple write operations.

The read operation can be attempted without waiting within an ISR, a task or
a fiber context. The write operation can occur within an ISR, a task or a
fiber context. A fiber waiting on LIFO data yields, while a task will busy
wait on the data keeping the CPU idle.

FIFOs
-----

FIFOs are specialized stacks where the first data that enters the stack is
also the first data to leave it.  Similarly to LIFOs, the nanokernel FIFOs
can get and put data of any size. The write operation allocates memory and
reserves the first word for the next pointer. Unlike the LIFOs, the read
operation is passed a pointer to the data structure. Thus, FIFOs support
multiple read and write operations.

The read operation can be attempted without waiting within an ISR, a task or
a fiber context. The write operation can occur within an ISR, a task or a
fiber context. Multiple fibers can wait on data from a FIFO, yielding until
marked as runnable. Multiple tasks can also wait on data from a FIFO but
they will busy wait keeping the CPU idle. The first waiting fiber gets the
data and is marked as runnable. Once there are no fibers waiting on data,
the tasks will get the data.

Nanokernel Timers
=================

The nanokernel implements a timer with data structure that allows for the
execution of a defined function at a defined time. The tick frequency can be
set with a configuration option and the timer driver sets the number of
clock cycles per tick. The nanokernel supports high resolution timers and
timers running in tickless mode. A simple API is provided to track the
elapsed ticks.

Timers can only be used from within a fiber or task context. Timers contain
a data pointer where a private pointer and user data can be stored. Timers
can be started, stopped or tested to see if they have expired. A fiber
context waiting on a timer to expire will yield and be marked as not runnable
. A task waiting on a timer will busy wait keeping the CPU idle.

Nanokernel Example Application: Hello World!
********************************************

The Hello World! example is a simple application that demonstrates the basic
sanity of the |codename|'s nanokernel. The background task and a fiber
execution contexts take turns printing a greeting on the console. Timers and
semaphores control the rate at which the messages are generated. The example
shows the correct operation of the of the scheduling, timing and
communication of the nanokernel.

This section focuses solely on the nanokernel implementation of the Hello
World example. The code samples are taken from
:file:`forto-collab/doc/doxygen/hello_commented.c`.

First, the needed nanokernel objects are defined:

.. literalinclude:: ../doxygen/hello_commented.c
      :language: c
      :lines: 128-140
      :emphasize-lines: 1, 2, 7, 10, 13
      :linenos:

The first two lines show how the timer is configured. Line 7 declares a
stack of a predefined size. Lines 10 and 13 declare a semaphore for each
execution context, one for the task and one for the fiber.

The task context executes the following code:

.. literalinclude:: ../doxygen/hello_commented.c
      :language: c
      :lines: 180-200
      :emphasize-lines: 12, 14-16, 18
      :linenos:

The task prints out Hello World! with line 12. Lines 14-16 have the task
start a timer, wait until it expires and then signal the semaphore of the
fiber. With line 18 the task waits until its semaphore is signaled once the
fiber yields.

The fiber context executes the following code:

.. literalinclude:: ../doxygen/hello_commented.c
      :language: c
      :lines: 152-170
      :emphasize-lines: 5, 7, 11, 13, 15-17
      :linenos:

The fiber initializes a semaphore and a timer in lines 5 and 7 respectively.
The fiber checks the semaphore and waits until it is signaled by the task
with line 11. Once the semaphore was signaled, the fiber prints out Hello
World! on the console with line 13. Lines 15 and 16 start a timer and wait
until it expires to continue. Lastly, line 17 signals the semaphore of the
task context.

The following figure shows the objects of the two different execution
contexts at work:

.. _architecture_nanokernel_1.svg:

.. figure:: /figures/architecture_nanokernel_1.svg
   :scale: 75 %
   :alt: Hello World! Example

   Graphic representation of the Hello World! example.