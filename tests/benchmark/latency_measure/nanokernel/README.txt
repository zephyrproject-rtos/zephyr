Title: Latency Measurement

Description:

This benchmark measures the latency of selected nanokernel features.

IMPORTANT: The results below were generated using a simulation environment,
and may not reflect the results that will be generated using other
environments (simulated or otherwise).

--------------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
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
|                                    E N D                                    |
|-----------------------------------------------------------------------------|
