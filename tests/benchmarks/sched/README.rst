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
