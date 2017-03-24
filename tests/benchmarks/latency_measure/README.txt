Title: Latency Measurement

Description:

This benchmark measures the latency of selected capabilities

IMPORTANT: The sample output below was generated using a simulation
environment, and may not reflect the results that will be generated using other
environments (simulated or otherwise).


Sample Output:

***** BOOTING ZEPHYR OS v1.7.99 - BUILD: Mar 24 2017 22:46:05 *****
|-----------------------------------------------------------------------------|
|                            Latency Benchmark                                |
|-----------------------------------------------------------------------------|
|  tcs = timer clock cycles: 1 tcs is 10 nsec                                 |
|-----------------------------------------------------------------------------|
| 1 - Measure time to switch from ISR back to interrupted thread              |
| switching time is 12591 tcs = 125910 nsec                                   |
|-----------------------------------------------------------------------------|
| 2 - Measure time from ISR to executing a different thread (rescheduled)       |
| switch time is 8344 tcs = 83440 nsec                                        |
|-----------------------------------------------------------------------------|
| 3 - Measure average time to signal a sema then test that sema               |
| Average semaphore signal time 63 tcs = 638 nsec                             |
| Average semaphore test time 49 tcs = 498 nsec                               |
|-----------------------------------------------------------------------------|
| 4- Measure average time to lock a mutex then unlock that mutex              |
| Average time to lock the mutex 107 tcs = 1078 nsec                          |
| Average time to unlock the mutex 92 tcs = 929 nsec                          |
|-----------------------------------------------------------------------------|
| 5 - Measure average context switch time between threads using (k_yield)       |
| Average thread context switch using yield 110 tcs = 1107 nsec                 |
|-----------------------------------------------------------------------------|
| 6 - Measure average context switch time between threads (coop)              |
| Average context switch time is 88 tcs = 882 nsec                            |
|-----------------------------------------------------------------------------|
===================================================================
PROJECT EXECUTION SUCCESSFUL
