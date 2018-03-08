.. _alerts_v2:

Alerts
######

An :dfn:`alert` is a kernel object that allows an application to perform
asynchronous signaling when a condition of interest occurs.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of alerts can be defined. Each alert is referenced by
its memory address.

An alert has the following key properties:

* An **alert handler**, which specifies the action to be taken
  when the alert is signaled. The action may instruct the system workqueue
  to execute a function to process the alert, mark the alert as pending
  so it can be processed later by a thread, or ignore the alert.

* An **pending count**, which records the number of pending alerts
  that have yet to be received.

* An **count limit**, which specifies the maximum number of pending alerts
  that will be recorded.

An alert must be initialized before it can be used. This establishes
its alert handler and sets the pending count to zero.

Alert Lifecycle
===============

An ISR or a thread signals an alert by **sending** the alert
when a condition of interest occurs that cannot be handled by the detector.

Each time an alert is sent, the kernel examines its alert handler
to determine what action to take.

* :c:macro:`K_ALERT_IGNORE` causes the alert to be ignored.

* :c:macro:`K_ALERT_DEFAULT` causes the pending count to be incremented,
  unless this would exceed the count limit.

* Any other value is assumed to be the address of an alert handler function,
  and is invoked by the system workqueue thread. If the function returns
  zero, the signal is deemed to have been consumed; otherwise the pending
  count is incremented, unless this would exceed the count limit.

  The kernel ensures that the alert handler function is executed once
  for each time an alert is sent, even if the alert is sent multiple times
  in rapid succession.

A thread accepts a pending alert by **receiving** the alert.
This decrements the pending count. If the pending count is currently zero,
the thread may choose to wait for the alert to become pending.
Any number of threads may wait for a pending alert simultaneously;
when the alert is pended it is accepted by the highest priority thread
that has waited longest.

.. note::
    A thread must processes pending alerts one at a time. The thread
    cannot receive multiple pending alerts in a single operation.

Comparison to Unix-style Signals
================================

Zephyr alerts are somewhat akin to Unix-style signals, but have a number of
significant differences. The most notable of these are:

* A Zephyr alert cannot be blocked; it is always delivered to its alert
  handler immediately.

* A Zephyr alert pends *after* it has been delivered to its alert handler,
  and only if an alert handler function does not consume the alert.

* Zephyr has no predefined alerts or actions. All alerts are application
  defined, and all have a default action that pends the alert.

Implementation
**************

Defining an Alert
=================

An alert is defined using a variable of type :c:type:`struct k_alert`.
It must then be initialized by calling :cpp:func:`k_alert_init()`.

The following code defines and initializes an alert. The alert allows
up to 10 unreceived alert signals to pend before it begins to ignore
new pending alerts.

.. code-block:: c

    extern int my_alert_handler(struct k_alert *alert);

    struct k_alert my_alert;

    k_alert_init(&my_alert, my_alert_handler, 10);

Alternatively, an alert can be defined and initialized at compile time
by calling :c:macro:`K_ALERT_DEFINE`.

The following code has the same effect as the code segment above.

.. code-block:: c

    extern int my_alert_handler(struct k_alert *alert);

    K_ALERT_DEFINE(my_alert, my_alert_handler, 10);

Signaling an Alert
==================

An alert is signaled by calling :cpp:func:`k_alert_send()`.

The following code illustrates how an ISR can signal an alert
to indicate that a key press has occurred.

.. code-block:: c

    extern int my_alert_handler(struct k_alert *alert);

    K_ALERT_DEFINE(my_alert, my_alert_handler, 10);

    void keypress_interrupt_handler(void *arg)
    {
        ...
        k_alert_send(&my_alert);
        ...
    }

Handling an Alert
=================

An alert handler function is used when a signaled alert should not be ignored
or immediately pended. It has the following form:

.. code-block:: c

    int <function_name>(struct k_alert *alert)
    {
        /* catch the alert signal; return zero if the signal is consumed, */
        /* or non-zero to let the alert pend                              */
        ...
    }

The following code illustrates an alert handler function that processes
key presses detected by an ISR (as shown in the previous section).

.. code-block:: c

    int my_alert_handler(struct k_alert *alert_id_is_unused)
    {
        /* determine what key was pressed */
        char c = get_keypress();

        /* do complex processing of the keystroke */
	...

        /* signaled alert has been consumed */
        return 0;
    }

Accepting an Alert
==================

A pending alert is accepted by a thread by calling :cpp:func:`k_alert_recv()`.

The following code is an alternative to the code in the previous section.
It uses a dedicated thread to do very complex processing
of key presses that would otherwise monopolize the system workqueue.
The alert handler function is now used only to filter out unwanted key press
alerts, allowing the dedicated thread to wake up and process key press alerts
only when a numeric key is pressed.

.. code-block:: c

    int my_alert_handler(struct k_alert *alert_id_is_unused)
    {
        /* determine what key was pressed */
        char c = get_keypress();

        /* signal thread only if key pressed was a digit */
        if ((c >= '0') && (c <= '9')) {
            /* save key press information */
            ...
            /* signaled alert should be pended */
            return 1;
        } else {
            /* signaled alert has been consumed */
            return 0;
        }
    }

    void keypress_thread(void *unused1, void *unused2, void *unused3)
    {
        /* consume numeric key presses */
        while (1) {

            /* wait for a key press alert to pend */
            k_alert_recv(&my_alert, K_FOREVER);

            /* process saved key press, which must be a digit */
            ...
        }
    }

Suggested Uses
**************

Use an alert to minimize ISR processing by deferring interrupt-related
work to a thread to reduce the amount of time interrupts are locked.

Use an alert to allow the kernel's system workqueue to handle an alert,
rather than defining an application thread to handle it.

Use an alert to allow the kernel's system workqueue to preprocess an alert,
prior to letting an application thread handle it.

Configuration Options
*********************

Related configuration options:

* None.

APIs
****

The following alert APIs are provided by :file:`kernel.h`:

* :c:macro:`K_ALERT_DEFINE`
* :cpp:func:`k_alert_init()`
* :cpp:func:`k_alert_send()`
* :cpp:func:`k_alert_recv()`
