.. _application_porting_guide:

Legacy Applications Porting Guide
##################################


.. note::

   This document is still work in progress.

This guide will help you move your applications from the nanokerne/microkernel
model to the unified kernel. The unified kernel was introduced with
:ref:`zephyr_1.6` which was released late 2016.

A list of the major changes that came with the unified kernel can be found in
the section :ref:`changes_v2`.


API Changes
***********

As described in the section :ref:`kernel_api_changes` the kernel now has one
unified and consistent API with new naming.

An application using the old APIs can still be compiled using a legacy interface
that translates old APIs to the new APIs. This legacy interface maintained in
:file:`include/legacy.h` can be used as a guide when porting a legacy
application to the new kernel.

Same Arguments
==============

In many cases, a simple search and replace is enough to move from the legacy to
the new APIs, for example:

* :cpp:func:`task_abort()` -> :cpp:func:`k_thread_abort()`
* :cpp:func:`task_sem_count_get()` -> :cpp:func:`k_sem_count_get()`

Additional Arguments
====================
The number of arguments to some APIs have changed,

* :cpp:func:`nano_sem_init()` -> :cpp:func:`k_sem_init()`

  This function now accepts 2 additional arguments:

  - Initial semaphore count
  - Permitted semaphore count

  When porting your application, make sure you have set the right arguments. For
  example, calls to the old API:

  .. code-block:: c

     nano_sem_init(sem)

  depending on the usage becomes in most cases:

  .. code-block:: c

     k_sem_init(sem, 0, UINT_MAX);

Return Codes
============

Many kernel APIs now return 0 to indicate success and a non-zero error code
to indicate the reason for failure. You should pay special attention to this
change when checking for return codes from kernel APIs, for example:

* :cpp:func:`k_sem_take()` now returns 0 on on success, in the legacy API
  :cpp:func:`nano_sem_take()` returned 1 when a semaphore is available.


Application Porting
*******************

The existing :ref:`synchronization_sample` from the Zephyr tree will be used to
guide you with porting a legacy application to the new kernel.

The code has been ported to the new kernel and is shown below:

.. literalinclude:: sync_v2.c
   :linenos:
   :language: c
   :lines: 9-

Porting a Nanokernel Application
=================================

Below is the code for the application using the legacy kernel:

.. literalinclude:: sync_v1_nano.c
   :linenos:
   :language: c
   :lines: 9-

Porting a Microkernel Application
=================================

The MDEF feature of the legacy kernel has been eliminated. Consequently, all
kernel objects are now defined directly in code.

Below is the code for the application using the legacy kernel:

.. literalinclude:: sync_v1.c
   :linenos:
   :language: c
   :lines: 9-


A microkernel application defines the used objects in an MDEF file, for this
porting sample using the :ref:`synchronization_sample`, the file is shown below:

.. literalinclude:: v1.mdef
   :linenos:

In the unified kernel the semaphore will be defined in the code as follows:

.. literalinclude:: sync_v2.c
   :linenos:
   :language: c
   :lines: 51-54

The threads (previously named tasks) are defined in the code as follows, for
thread A:

.. literalinclude:: sync_v2.c
   :linenos:
   :language: c
   :lines: 88-89

Thread B (taskB in the microkernel) will be spawned dynamically from thread A
(See :ref:`spawning_thread` section):

.. literalinclude:: sync_v2.c
   :linenos:
   :language: c
   :lines: 80-82
