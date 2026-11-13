.. SPDX-FileCopyrightText: Copyright 2026 NXP
.. SPDX-License-Identifier: Apache-2.0

Long application timeouts across timer maintenance
##################################################

These software-only tests connect the unmodified generic timer core to the
real kernel timeout queue and application ``k_timer`` callbacks. The announce
entry point is not replaced.

The test application supplies its own system clock and disables the platform
SysTick driver. Only the raw 32-bit counter and compare/reload writes are
controlled. An independent 64-bit input timeline advances the counter to each
armed alarm, and IRQ offload delivers the timer ISR in interrupt context.
The declared 4000 Hz counter has four cycles per kernel tick. QEMU icount is
disabled so its one-second arm policy does not hide the counter-span boundary.

Each long application timeout spans more than ``UINT32_MAX`` ticks, requiring
multiple maintenance interrupts and raw-counter wraps. The first three
maintenance interrupts are delivered five cycles late to exercise retained
fractional cycles as well as late-announce headroom. Final application
deadlines are delivered without added latency.

The cases check that:

* A long relative timeout expires exactly once at its expected tick, including
  the documented one-tick round-up when armed outside an announce callback.
* A short relative timeout added after several maintenance interrupts fires
  first, without changing the pending long absolute timeout's deadline.
* A short relative timeout added after the long timer expires, at a sub-tick
  phase, is neither advanced nor delayed by the old maintenance baseline.

Every delivered interrupt and explicit time advance checks kernel uptime and
both cycle getters against the input timeline. Callback counts, ordering,
ISR context, and exact firing ticks/cycles are checked independently of the
timer's reported expiry. The kernel queue is the default delta list.

Build the two software configurations from the Zephyr tree with::

   west build -b mps2/an385 tests/kernel/timer/timeout_long -d build/long-compare
   west build -d build/long-compare -t run
   west build -b mps2/an385 tests/kernel/timer/timeout_long -d build/long-reload -- \
       -DCONFIG_TEST_TIMEOUT_LONG_RELOAD=y
   west build -d build/long-reload -t run

Use an independent host timeout of 30 seconds; the simulated clock must not
be its own stall detector. The event pump also has a bounded interrupt count.
Reported ztest durations use the synthetic clock, not host elapsed time.

Do not flash this image to a physical board: time advances only when the test
requests it. This is not evidence for real register/IRQ behavior, natural
hardware wrap, PM, tickful operation, other kernel queue backends, or an
announce count near the interface's maximum.
