.. _microkernel_fibers:

Fiber Services
##############

Concepts
********

A fiber is a lightweight, non-preemptible thread of execution that implements
a portion of an application's processing. It is is normally used when writing
device drivers and other performance critical work.

A microkernel application can use all of the fiber capabilities that are
available to a nanokernel application; for more information see
:ref:`Nanokernel Fiber Services <nanokernel_fibers>`.

While a fiber often uses one or more nanokernel object types to carry
out its work, it can also interact with microkernel events and semaphores
to a limited degree. For example, a fiber can signal a task by giving a
microkernel semaphore, but it cannot take a microkernel semaphore. For more
information see :ref:`microkernel_events` and :ref:`microkernel_semaphores`.


.. _microkernel_server_fiber:

Microkernel Server Fiber
========================

The microkernel automatically spawns a system thread, known as the
*microkernel server* fiber, which performs most operations involving
microkernel objects; see :ref:`Microkernel Server <microkernel_server>`.

Both the fiber's stack size and scheduling priority can be configured,
using the :option:`MICROKERNEL_SERVER_STACK_SIZE` and
:option:`MICROKERNEL_SERVER_PRIORITY` configuration options, respectively.

By default, the microkernel server has priority 0 (i.e. highest priority).
However, this can be changed so that the nanokernel scheduler gives precedence
to other fibers, such as time-sensitive device driver or application fibers.
