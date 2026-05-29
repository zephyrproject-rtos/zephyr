Userspace Scheduler Microbenchmark
##################################

This is a scheduler microbenchmark, designed to measure minimum
latencies (not scaling performance) of specific low level scheduling
primitives independent of overhead from application or API
abstractions. Contrary to the non-userspace version, it runs threads
in userspace with different memory domains.

 It works very simply: a main thread creates n "yielders"
threads at a higher priority, from this initial state:

1. The main thread starts all yielders
2. Each yielder yields k times
5. The main thread joins all yielders

This is run for multiples values of n, reporting each time the
average time taken for a yield context switch.
