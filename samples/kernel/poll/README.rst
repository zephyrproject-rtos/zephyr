.. Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:code-sample:: kernel-poll
   :name: Kernel event dispatcher
   :relevant-api: poll_apis

   Handle multiple kernel event sources from one dispatcher thread using ``k_poll()``.

Overview
********

This sample demonstrates how a single dispatcher thread can wait for several
different kernel objects without actively checking each object. It models a
small sensor application with the following asynchronous event sources:

* a message queue containing samples produced from a timer expiry function;
* a semaphore representing a control request; and
* a poll signal carrying the result of a shutdown request.

The dispatcher schedules housekeeping every 120 ms. Each call to
:c:func:`k_poll` waits only until the next housekeeping deadline. After waking,
the dispatcher runs housekeeping if it is due, even when an event woke it.
It then checks every event because several can be ready at once, consumes the
associated kernel objects, and resets the event states before polling again.

The sample timer produces four readings 80 ms apart. The control and shutdown
requests follow. Every gap between inputs is shorter than the housekeeping
interval, so the sample demonstrates housekeeping while events keep arriving.

This pattern is useful when one thread owns several uncontended objects. It
can reduce the number of application threads and their stack memory, at the
cost of enabling the polling infrastructure with :kconfig:option:`CONFIG_POLL`.

Building and Running
********************

Build and run the sample on :zephyr:board:`qemu_cortex_m3` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/kernel/poll
   :host-os: unix
   :board: qemu_cortex_m3
   :goals: run
   :compact:

To build for another board, replace ``qemu_cortex_m3`` with the board name.

Sample Output
=============

The exact number and placement of housekeeping messages depend on scheduling.
The following output was captured on ``qemu_cortex_m3``:

.. code-block:: console

   Kernel event dispatcher started
   sample 0: value=20
   housekeeping 1
   sample 1: value=21
   sample 2: value=22
   housekeeping 2
   sample 3: value=23
   control request handled
   housekeeping 3
   shutdown requested: result=0
   dispatcher complete: samples=4 control=1 housekeeping=3

Exit QEMU by pressing :kbd:`CTRL+A` followed by :kbd:`x`.
