Latency Measurements
####################

This benchmark measures the average latency of selected kernel capabilities,
including:

* Context switch time between preemptive threads using k_yield
* Context switch time between cooperative threads using k_yield
* Time to switch from ISR back to interrupted thread
* Time from ISR to executing a different thread (rescheduled)
* Times to signal a semaphore then test that semaphore
* Times to signal a semaphore then test that semaphore with a context switch
* Times to lock a mutex then unlock that mutex
* Time it takes to create a new thread (without starting it)
* Time it takes to start a newly created thread
* Time it takes to suspend a thread
* Time it takes to resume a suspended thread
* Time it takes to abort a thread
* Measure average time to alloc memory from heap then free that memory

When userspace is enabled using the prj_user.conf configuration file, this benchmark will
where possible, also test the above capabilities using various configurations involving user
threads:

* Kernel thread to kernel thread
* Kernel thread to user thread
* User thread to kernel thread
* User thread to user thread

Sample output of the benchmark (without userspace enabled)::

        *** Booting Zephyr OS build v3.5.0-rc1-139-gdab69aeed11d ***
        START - Time Measurement
        Timing results: Clock frequency: 120 MHz
        Preemptive threads ctx switch via k_yield (K -> K)  :     519 cycles ,     4325 ns :
        Cooperative threads ctx switch via k_yield (K -> K) :     519 cycles ,     4325 ns :
        Switch from ISR back to interrupted thread          :     508 cycles ,     4241 ns :
        Switch from ISR to another thread (kernel)          :     554 cycles ,     4616 ns :
        Create kernel thread from kernel thread             :     396 cycles ,     3308 ns :
        Start kernel thread from kernel thread              :     603 cycles ,     5033 ns :
        Suspend kernel thread from kernel thread            :     599 cycles ,     4992 ns :
        Resume kernel thread from kernel thread             :     547 cycles ,     4558 ns :
        Abort kernel thread from kernel thread              :     339 cycles ,     2825 ns :
        Give a semaphore (no waiters) from kernel thread    :     134 cycles ,     1116 ns :
        Take a semaphore (no blocking) from kernel thread   :      53 cycles ,      441 ns :
        Take a semaphore (context switch K -> K)            :     689 cycles ,     5742 ns :
        Give a semaphore (context switch K -> K)            :     789 cycles ,     6575 ns :
        Lock a mutex from kernel thread                     :      94 cycles ,      783 ns :
        Unlock a mutex from kernel thread                   :      24 cycles ,      200 ns :
        Average time for heap malloc                        :     620 cycles ,     5166 ns :
        Average time for heap free                          :     431 cycles ,     3591 ns :
        ===================================================================
        PROJECT EXECUTION SUCCESSFUL

Sample output of the benchmark (with userspace enabled)::

        *** Booting Zephyr OS build v3.5.0-rc1-139-gdab69aeed11d ***
        START - Time Measurement
        Timing results: Clock frequency: 120 MHz
        Preemptive threads ctx switch via k_yield (K -> K)  :    1195 cycles ,     9958 ns :
        Preemptive threads ctx switch via k_yield (U -> U)  :    1485 cycles ,    12379 ns :
        Preemptive threads ctx switch via k_yield (K -> U)  :    1390 cycles ,    11587 ns :
        Preemptive threads ctx switch via k_yield (U -> K)  :    1289 cycles ,    10749 ns :
        Cooperative threads ctx switch via k_yield (K -> K) :    1185 cycles ,     9875 ns :
        Cooperative threads ctx switch via k_yield (U -> U) :    1475 cycles ,    12295 ns :
        Cooperative threads ctx switch via k_yield (K -> U) :    1380 cycles ,    11504 ns :
        Cooperative threads ctx switch via k_yield (U -> K) :    1280 cycles ,    10666 ns :
        Switch from ISR back to interrupted thread          :    1130 cycles ,     9416 ns :
        Switch from ISR to another thread (kernel)          :    1184 cycles ,     9874 ns :
        Switch from ISR to another thread (user)            :    1390 cycles ,    11583 ns :
        Create kernel thread from kernel thread             :     985 cycles ,     8208 ns :
        Start kernel thread from kernel thread              :    1275 cycles ,    10625 ns :
        Suspend kernel thread from kernel thread            :    1220 cycles ,    10167 ns :
        Resume kernel thread from kernel thread             :    1193 cycles ,     9942 ns :
        Abort kernel thread from kernel thread              :    2555 cycles ,    21292 ns :
        Create user thread from kernel thread               :     849 cycles ,     7083 ns :
        Start user thread from kernel thread                :    6715 cycles ,    55960 ns :
        Suspend user thread from kernel thread              :    1585 cycles ,    13208 ns :
        Resume user thread from kernel thread               :    1383 cycles ,    11525 ns :
        Abort user thread from kernel thread                :    2420 cycles ,    20167 ns :
        Create user thread from user thread                 :    2110 cycles ,    17584 ns :
        Start user thread from user thread                  :    7070 cycles ,    58919 ns :
        Suspend user thread from user thread                :    1784 cycles ,    14874 ns :
        Resume user thread from user thread                 :    1740 cycles ,    14502 ns :
        Abort user thread from user thread                  :    3000 cycles ,    25000 ns :
        Start kernel thread from user thread                :    1630 cycles ,    13583 ns :
        Suspend kernel thread from user thread              :    1420 cycles ,    11833 ns :
        Resume kernel thread from user thread               :    1550 cycles ,    12917 ns :
        Abort kernel thread from user thread                :    3135 cycles ,    26125 ns :
        Give a semaphore (no waiters) from kernel thread    :     160 cycles ,     1333 ns :
        Take a semaphore (no blocking) from kernel thread   :      95 cycles ,      791 ns :
        Give a semaphore (no waiters) from user thread      :     380 cycles ,     3166 ns :
        Take a semaphore (no blocking) from user thread     :     315 cycles ,     2625 ns :
        Take a semaphore (context switch K -> K)            :    1340 cycles ,    11167 ns :
        Give a semaphore (context switch K -> K)            :    1460 cycles ,    12167 ns :
        Take a semaphore (context switch K -> U)            :    1540 cycles ,    12838 ns :
        Give a semaphore (context switch U -> K)            :    1800 cycles ,    15000 ns :
        Take a semaphore (context switch U -> K)            :    1690 cycles ,    14084 ns :
        Give a semaphore (context switch K -> U)            :    1650 cycles ,    13750 ns :
        Take a semaphore (context switch U -> U)            :    1890 cycles ,    15756 ns :
        Give a semaphore (context switch U -> U)            :    1990 cycles ,    16583 ns :
        Lock a mutex from kernel thread                     :     105 cycles ,      875 ns :
        Unlock a mutex from kernel thread                   :      17 cycles ,      141 ns :
        Lock a mutex from user thread                       :     330 cycles ,     2750 ns :
        Unlock a mutex from user thread                     :     255 cycles ,     2125 ns :
        Average time for heap malloc                        :     606 cycles ,     5058 ns :
        Average time for heap free                          :     422 cycles ,     3516 ns :
        ===================================================================
        PROJECT EXECUTION SUCCESSFUL
