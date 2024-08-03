.. _signals:

Signal API
##########

The signal API supports signalling between threads.

.. contents::
    :local:
    :depth: 2

Concepts
********

The signal API provides a way for one thread to notify another thread that an
event has occurred using a predefined integer value
(see :zephyr_file:`include/zephyr/posix/signal.h`).

Zephyr's signal API can be broken down into functions that manipulate and
query *signal sets* (:c:struct:`k_sig_set`), and functions that are used to
send and receive signals (see :ref:`Signal API Implementation <signals_implementation>`).

Standard vs Real-time Signals
=============================

Signals may be divided into two categories; *standard* and *real-time*. The
main differences between the two are

* ISO C and POSIX do not specify whether standard signals are queued or merged.
* ISO C and POSIX do not specify the priority of standard signals.

To illustrate the former point, if ``SIGALRM`` is sent twice to an
application, the implementation may merge those into one signal or queue them.
On the other hand, Real-time signals are always queued in the order they are
generated, and are never merged.

Standard signals therefore have a shortcoming at the specification level; there
can be no guarantee of determinism if a signal generated at one time instance
is merged with another signal generated at another time instance. Zephyr removes
this ambiguity by delivering all signals in the order they are generated, and
not merging any signals. Zephyr's handling of standard signals is similar
to how Zephyr handles real-time signals.

.. note::
    Zephyr queues all signals in the order they are generated, and never
    merges any signals (real-time or otherwise).

To address the latter point, real-time signals have a priority with respect to
other real-time signals. However, the POSIX specification does not clarify the
priority of real-time signals with respect to standard signals. To address
this ambiguity, Zephyr always delivers unmasked, real-time signals in
prioritized order (lowest-numbered signal) and *then* delivers standard signals
in the order they are generated.

.. note::
    Zephyr prioritizes real-time signals before standard signals.

Limitations
===========

As per ISO C and POSIX, signals should be delivered and handled to the
application asynchronously if they are not masked. Currently, Zephyr
does not support asynchronous signal delivery, as that may introduce
elements of non-determinism. Therefore, all signals should be received
synchronously via calls to :c:func:`k_sig_timedwait`.

.. _signals_implementation:

Implementation
**************

Signal Sets
===========

When :kconfig:option:`CONFIG_SIGNAL` is enabled, every :c:struct:`k_thread` is
created with an initially-empty :c:struct:`k_sig_set` of masked signals. If a
signal is masked (i.e. part of the set), the corresponding bit in the signal
set is 1, otherwise it is 0. Thus an initially empty :c:struct:`k_sig_set` is
zero-valued. Signal sets should not be modified directly, but indirectly via
the functions provided in the signal API.

Before a signal set can be used, it must be initialized via
:c:func:`k_sig_emptyset` or :c:func:`k_sig_fillset`, which either create an
empty or full signal set, respectively. Using a signal set without first
initializing it via either of the two aforementioned functions results in
undefined behaviour.

.. code-block:: c
    :caption: Initialize a :c:struct:`k_sig_set`

    struct k_sig_set set;

    k_sig_emptyset(&set);
    /* or */
    k_sig_fillset(&set);


Signals may then be added via :c:func:`k_sig_addset`, removed via
:c:func:`k_sig_delset`, and tested for via :c:func:`k_sig_ismember`.

.. code-block:: c
    :caption: Add, remove, or query signals in a :c:struct:`k_sig_set`

    k_sig_addset(&set, SIGINT);
    /* or */
    k_sig_delset(&set, SIGTERM);

    if (k_sig_ismember(&set, SIGINT) == 1) {
        /* SIGINT is in the set */
    }

The Signal Mask
===============

The signal mask belonging to each thread may only be modified by the thread
itself using :c:func:`k_sig_mask`. The signal mask may be manipulated in one
of three possible ways

1. :c:macro:`K_SIG_BLOCK` - the signals in the set provided are added to the mask
2. :c:macro:`K_SIG_UNBLOCK` - the signals in the set provided are removed from
   the mask
3. :c:macro:`K_SIG_SETMASK` - the mask is replaced by the set provided.

In order to retrieve that current value of the signal mask, or to obtain the
value of the set before it was modified, pass a non-``NULL`` pointer for the
``oset`` (old set) argument to :c:func:`k_sig_mask`.

.. code-block:: c
    :caption: Manipulate the signal mask of the current thread

    struct k_sig_set set, oset;

    k_sig_emptyset(&set);
    k_sig_addset(&set, SIGINT);

    k_sig_mask(K_SIG_BLOCK, &set, NULL);
    /* SIGINT is now blocked */

    /* Retrieve the current signal mask */
    k_sig_mask(-1, NULL, &oset);
    /* oset contains SIGINT and other previously masked signals */

Sending Signals
===============

Signals are queued to a thread using :c:func:`k_sig_queue()`. The first
argument to this function is of type :c:type:`k_pid_t`. Although the name of
:c:type:`k_pid_t` may imply the opposite, Zephyr does not yet support
processes and :c:type:`k_pid_t` is synonymous with :c:type:`k_tid_t`. Along
with the signal number, :c:func:`k_sig_queue()` also accepts a
:c:union:`k_sig_val` which may be used to pass additional information to the
receiving thread in the form of a pointer or integer value.

.. code-block:: c
    :caption: Send a signal to the current thread

    /* arbitrary data to be sent with the signal */
    int data = 1234;
    extern uint8_t shared_resource[4200];
    k_pid_t pid = (k_pid_t) k_current_get();
    union k_sig_val val = {
        .sival_int = data,
    };

    /* use an integer value */
    k_sig_queue(pid, SIGUSR1, val);

    /* use a pointer value */
    val.sival_ptr = &shared_resource;
    k_sig_queue(pid, SIGUSR2, val);

Receiving Signals
=================

A thread may query for the signals pending delivery via
:c:func:`k_sig_pending`. This function takes only one argument, a pointer to a
signal set, which will be filled with the signals pending delivery.

To receive a signal, and wait for a configurable amount of time for one to be
delivered, the function :c:func:`k_sig_timedwait` is used.

.. code-block:: c
    :caption: Receive a signal

    struct k_sig_set set;
    struct k_sig_info info;

    k_sig_emptyset(&set);
    k_sig_addset(&set, SIGUSR1);
    k_sig_timedwait(&set, &info, K_FOREVER);
    /* SIGUSR1 has been received */

    /*
     * Additional data may be in info.si_value.sival_int or
     * info.si_value.sival_ptr
     */

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_SIGNAL`
* :kconfig:option:`CONFIG_SIGNAL_QUEUE_SIZE`
* :kconfig:option:`CONFIG_SIGNAL_SET_SIZE`

API Reference
*************

.. doxygengroup:: k_sig_apis
