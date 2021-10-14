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
* Measure average time to alloc memory from heap then free that memory


Sample output of the benchmark::

        *** Booting Zephyr OS build zephyr-v2.6.0-1119-g378a1e082ac5  ***
        START - Time Measurement
        Timing results: Clock frequency: 1000 MHz
        Average thread context switch using yield                   :    9060 cycles ,     9060 ns
        Average context switch time between threads (coop)          :    9503 cycles ,     9503 ns
        Switch from ISR back to interrupted thread                  :   14208 cycles ,    14208 ns
        Time from ISR to executing a different thread               :    9664 cycles ,     9664 ns
        Time to create a thread (without start)                     :    3968 cycles ,     3968 ns
        Time to start a thread                                      :   12064 cycles ,    12064 ns
        Time to suspend a thread                                    :   12640 cycles ,    12640 ns
        Time to resume a thread                                     :   12096 cycles ,    12096 ns
        Time to abort a thread (not running)                        :    2208 cycles ,     2208 ns
        Average semaphore signal time                               :    8928 cycles ,     8928 ns
        Average semaphore test time                                 :    2048 cycles ,     2048 ns
        Semaphore take time (context switch)                        :   13472 cycles ,    13472 ns
        Semaphore give time (context switch)                        :   18400 cycles ,    18400 ns
        Average time to lock a mutex                                :    3072 cycles ,     3072 ns
        Average time to unlock a mutex                              :    9251 cycles ,     9251 ns
        Average time for heap malloc                                :   13056 cycles ,    13056 ns
        Average time for heap free                                  :    7776 cycles ,     7776 ns
        ===================================================================
        PROJECT EXECUTION SUCCESSFUL
