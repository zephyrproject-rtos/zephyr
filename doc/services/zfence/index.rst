.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. _zfence:

Zfence
######

.. contents::
   :local:
   :depth: 2

Zfence provides a reusable software timeline for ordering asynchronous work.
A producer reserves a sequence value, publishes that value with its work, and
signals completion later. Consumers can wait for or query a particular sequence
value. A fence is not reset between operations; its sequence values advance for
the lifetime of the fence.

Use Zfence when several consumers need to observe completion of ordered work,
including when the producer completes work before a consumer begins waiting.
It is not a queue and does not retain a distinct completion result for every
sequence value.

Configuration
*************

Enable :kconfig:option:`CONFIG_ZFENCE` to include the subsystem. It selects
:kconfig:option:`CONFIG_EVENTS`, which provides the event used to wake waiting
threads.

By default, wait contexts use one system-wide pool with a maximum of
:c:macro:`ZFENCE_MAX_SYNC_WAITERS` contexts. Enable
:kconfig:option:`CONFIG_ZFENCE_PER_FENCE_WAIT_CTX` to give each fence an
independent pool of the same size.

Timeline model
**************

Each fence has a monotonically increasing completion sequence. Call
:c:func:`zfence_next` to reserve a value before publishing work that refers to
it. Calling :c:func:`zfence_signal` advances completion by one. Calling
:c:func:`zfence_signal_seq` advances completion to a specified later value;
all earlier values become complete at the same time.

Reservations allow several operations to be in flight. A signal is not required
to have a preceding reservation, but reserving the value first lets a producer
give consumers a stable completion point before the work finishes. A later
signal makes intermediate values complete, so use separate fences for work
that must complete independently or out of order.

Results
*******

The fence retains the result associated with only its latest completed sequence
value. :c:func:`zfence_result` reports
:c:macro:`ZFENCE_RESULT_UNSIGNALED` for a future value,
:c:macro:`ZFENCE_RESULT_OVERWRITTEN` for an earlier completed value whose
result is no longer retained, or the stored result for the current value.
Applications can use their own result values except for the two reserved
sentinels.

Waiting
*******

Each synchronous waiter owns a :c:type:`zfence_wait_context_t`. Initialize the
context with :c:func:`zfence_wait_ctx_init` before use, or let
:c:func:`zfence_wait` initialize it automatically. Release it with
:c:func:`zfence_wait_ctx_deinit` once it is no longer needed. The context must
remain valid while :c:func:`zfence_wait` is running. A context can be reused for
later waits after a wait completes or times out.

:c:func:`zfence_wait` first checks whether the requested value is already
complete. If it is not, the function blocks until the value is signaled or the
timeout expires. A successful wait stores the signal result through its optional
``result`` argument. A timeout returns ``-ETIMEDOUT`` and does not write that
argument.

Example
*******

The producer reserves and publishes the completion point before starting work.
The consumer may begin waiting before or after the signal.

.. code-block:: c

   #include <zephyr/kernel.h>
   #include <zephyr/zfence/zfence.h>

   static ZFENCE_DEFINE(completion_fence);
   static zfence_wait_context_t completion_waiter;

   int wait_for_work(void)
   {
           uint64_t seq;
           int result;
           int ret;

           ret = zfence_wait_ctx_init(&completion_waiter, &completion_fence);
           if (ret != 0) {
                   return ret;
           }

           seq = zfence_next(&completion_fence);
           publish_work(seq);

           ret = zfence_wait(&completion_fence, &completion_waiter, seq,
                             K_MSEC(20), &result);
           if (ret == 0) {
                   return result;
           }

           return ret;
   }

   int work_completed(uint64_t seq, int result)
   {
           return zfence_signal_seq(&completion_fence, seq, result);
   }

The caller can call :c:func:`zfence_wait_ctx_deinit` when
``completion_waiter`` is no longer required.

API reference
*************

.. doxygengroup:: zfence
   :project: Zephyr
