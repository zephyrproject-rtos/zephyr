Latency Measurements
####################

This benchmark measures the latency of selected kernel capabilities, including:


* Measure time to switch from ISR back to interrupted thread
* Measure time from ISR to executing a different thread (rescheduled)
* Measure average time to signal a semaphore then test that semaphore
* Measure average time to signal a semaphore then test that semaphore with a context switch
* Measure average time to lock a mutex then unlock that mutex
* Measure average context switch time between threads using (k_yield)
* Measure average context switch time between threads (coop)
* Time it takes to suspend a thread
* Time it takes to resume a suspended thread
* Time it takes to create a new thread (without starting it)
* Time it takes to start a newly created thread


Sample output of the benchmark::

        *** Booting Zephyr OS build zephyr-v2.3.0-2257-g0f420483db07  ***
        START - Time Measurement
        Timing results: Clock frequency: 120 MHz
        Average thread context switch using yield                   :     420 cycles ,     3502 ns
        Average context switch time between threads (coop)          :     429 cycles ,     3583 ns
        Switch from ISR back to interrupted thread                  :     670 cycles ,     5583 ns
        Time from ISR to executing a different thread               :     570 cycles ,     4750 ns
        Time to create a thread (without start)                     :     360 cycles ,     3000 ns
        Time to start a thread                                      :     545 cycles ,     4541 ns
        Time to suspend a thread                                    :     605 cycles ,     5041 ns
        Time to resume a thread                                     :     660 cycles ,     5500 ns
        Time to abort a thread (not running)                        :     495 cycles ,     4125 ns
        Average semaphore signal time                               :     195 cycles ,     1626 ns
        Average semaphore test time                                 :      62 cycles ,      518 ns
        Semaphore take time (context switch)                        :     695 cycles ,     5791 ns
        Semaphore give time (context switch)                        :     845 cycles ,     7041 ns
        Average time to lock a mutex                                :      79 cycles ,      659 ns
        Average time to unlock a mutex                              :     370 cycles ,     3085 ns
        ===================================================================
        PROJECT EXECUTION SUCCESSFUL
