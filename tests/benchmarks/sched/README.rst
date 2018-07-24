Scheduler Microbenchmark
########################

This is a scheduler microbenchmark, designed to measure minimum
latencies (not scaling performance) of specific low level scheduling
primitives independent of overhead from application or API
abstractions.  It works very simply: a main thread creates a "partner"
thread at a higher priority, the partner then sleeps using
_pend_curr_irqlock().  From this initial state:

1. The main thread calls _unpend_first_thread()
2. The main thread calls _ready_thread()
3. The main thread calls k_yield()
   (the kernel switches to the partner thread)
4. The partner thread then runs and calls _pend_curr_irqlock() again
   (the kernel switches to the main thread)
5. The main thread returns from k_yield()

It then iterates this many times, reporting timestamp latencies
between each numbered step and for the whole cycle, and a running
average for all cycles run.

Note that because this involves no timer interaction (except, on some
architectures, k_cycle_get_32()), it works correctly when run in QEMU
using the -icount argument, which can produce 100% deterministic
behavior (not cycle-exact hardware simulation, but exactly N
instructions per simulated nanosecond).  You can enable this using an
environment variable (set at cmake time -- it's not enough to do this
for the subsequent make/ninja invocation, cmake needs to see the
variable itself):

    export QEMU_EXTRA_FLAGS="-icount shift=0,align=off,sleep=off"
