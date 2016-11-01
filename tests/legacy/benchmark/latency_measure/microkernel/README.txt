Title: Latency Measurement

Description:

This benchmark measures the latency of selected capabilities of both the
nanokernel and microkernel.

IMPORTANT: The sample output below was generated using a simulation
environment, and may not reflect the results that will be generated using other
environments (simulated or otherwise).

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make qemu

--------------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

--------------------------------------------------------------------------------

Sample Output:

|-----------------------------------------------------------------------------|
|                        Nanokernel Latency Benchmark                         |
|-----------------------------------------------------------------------------|
|  tcs = timer clock cycles: 1 tcs is N nsec                                  |
|-----------------------------------------------------------------------------|
| 1- Measure time to switch from fiber to ISR execution                       |
| switching time is NNNN tcs = NNNNN nsec                                     |
|-----------------------------------------------------------------------------|
| 2- Measure time to switch from ISR back to interrupted fiber                |
| switching time is NNNN tcs = NNNNN nsec                                     |
|-----------------------------------------------------------------------------|
| 3- Measure time from ISR to executing a different fiber (rescheduled)       |
| switching time is NNNN tcs = NNNNN nsec                                     |
|-----------------------------------------------------------------------------|
| 4- Measure average context switch time between fibers                       |
| Average context switch time is NNNN tcs = NNNNN nsec                        |
|-----------------------------------------------------------------------------|
| 5- Measure average time to lock then unlock interrupts                      |
| 5.1- When each lock and unlock is executed as a function call               |
| Average time for lock then unlock is NNNN tcs = NNNN nsec                   |
|                                                                             |
| 5.2- When each lock and unlock is executed as inline function call          |
| Average time for lock then unlock is NNN tcs = NNNN nsec                    |
|-----------------------------------------------------------------------------|
|-----------------------------------------------------------------------------|
|                        Microkernel Latency Benchmark                        |
|-----------------------------------------------------------------------------|
|  tcs = timer clock cycles: 1 tcs is N nsec                                  |
|-----------------------------------------------------------------------------|
| 1- Measure time to switch from ISR to back to interrupted task              |
| switching time is NNNN tcs = NNNNN nsec                                     |
|-----------------------------------------------------------------------------|
| 2- Measure time from ISR to executing a different task (rescheduled)        |
| switch time is NNNNN tcs = NNNNNN nsec                                      |
|-----------------------------------------------------------------------------|
| 3- Measure average time to signal a sema then test that sema                |
| Average semaphore signal time NNNNN tcs = NNNNNN nsec                       |
| Average semaphore test time NNNNN tcs = NNNNNN nsec                         |
|-----------------------------------------------------------------------------|
| 4- Measure average time to lock a mutex then unlock that mutex              |
| Average time to lock the mutex NNNNN tcs = NNNNNN nsec                      |
| Average time to unlock the mutex NNNNN tcs = NNNNNN nsec                    |
|-----------------------------------------------------------------------------|
| 5- Measure average context switch time between tasks using (task_yield)     |
| Average task context switch using yield NNNNN tcs = NNNNNN nsec             |
|-----------------------------------------------------------------------------|
|                                    E N D                                    |
|-----------------------------------------------------------------------------|
===================================================================
PROJECT EXECUTION SUCCESSFUL
