.. _microkernel_fibers:

Fiber Services
##############

Concepts
********

A :dfn:`fiber` is a lightweight, non-preemptible thread of execution that
implements a portion of an application's processing. Fiber-based services are
often used in device drivers and for performance-critical work.

A microkernel application can use all of the fiber capabilities that are
available to a nanokernel application; for more information see
:ref:`nanokernel_fibers`.

While a fiber often uses one or more nanokernel object types to carry
out its work, it also can interact with microkernel events and semaphores
to a limited degree. For example, a fiber can signal a task by giving a
microkernel semaphore, but it cannot take a microkernel semaphore. For more
information see :ref:`microkernel_events` and :ref:`microkernel_semaphores`.


.. _microkernel_server_fiber:

Microkernel Server Fiber
========================

The microkernel automatically spawns a system thread, known as the
*microkernel server* fiber, which performs most operations involving
microkernel objects. The nanokernel scheduler decides which fibers
get scheduled and when; it will schedule the microkernel server fiber
when there are no fibers of a higher priority.

By default, the microkernel server fiber has priority 0 (that is,
the highest priority). However, this can be changed. If you drop its
priority, the nanokernel scheduler will give precedence to other,
higher-priority fibers, such as time-sensitive device driver or
application fibers.

Both the fiber's stack size and scheduling priority can be configured
with the :option:`MICROKERNEL_SERVER_STACK_SIZE` and
:option:`MICROKERNEL_SERVER_PRIORITY` configuration options,
respectively.


See also :ref:`microkernel_server`.