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

The dispatcher also uses the :c:func:`k_poll` timeout to perform periodic
housekeeping. After ``k_poll()`` returns, it checks every event because more
than one event can be ready at the same time. It then consumes the associated
kernel object and resets the event state before polling again.

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

The exact number and placement of housekeeping messages depends on scheduling.
The following output was captured on ``qemu_cortex_m3``:

.. code-block:: console

   Kernel event dispatcher started
   sample 0: value=20
   housekeeping 1
   sample 1: value=21
   control request handled
   sample 2: value=22
   housekeeping 2
   sample 3: value=23
   housekeeping 3
   housekeeping 4
   housekeeping 5
   shutdown requested: result=0
   dispatcher complete: samples=4 control=1 timeouts=5 dropped=0

Exit QEMU by pressing :kbd:`CTRL+A` followed by :kbd:`x`.
