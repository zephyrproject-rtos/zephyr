Latency Measurements
####################

This benchmark measures the average latency of selected kernel capabilities,
including:

* Context switch time between preemptive threads using k_yield
* Context switch time between cooperative threads using k_yield
* Time to switch from ISR back to interrupted thread
* Time from ISR to executing a different thread (rescheduled)
* Time to signal a semaphore then test that semaphore
* Time to signal a semaphore then test that semaphore with a context switch
* Times to lock a mutex then unlock that mutex
* Time it takes to create a new thread (without starting it)
* Time it takes to start a newly created thread
* Time it takes to suspend a thread
* Time it takes to resume a suspended thread
* Time it takes to abort a thread
* Time it takes to add data to a fifo.LIFO
* Time it takes to retrieve data from a fifo.LIFO
* Time it takes to wait on a fifo.lifo.(and context switch)
* Time it takes to wake and switch to a thread waiting on a fifo.LIFO
* Time it takes to send and receive events
* Time it takes to wait for events (and context switch)
* Time it takes to wake and switch to a thread waiting for events
* Time it takes to push and pop to/from a k_stack
* Measure average time to alloc memory from heap then free that memory

When userspace is enabled, this benchmark will where possible, also test the
above capabilities using various configurations involving user threads:

* Kernel thread to kernel thread
* Kernel thread to user thread
* User thread to kernel thread
* User thread to user thread

The default configuration builds only for the kernel. However, additional
configurations can be enabled via the use of EXTRA_CONF_FILE.

For example, the following will build this project with userspace support:

    EXTRA_CONF_FILE="prj.userspace.conf" west build -p -b <board> <path to project>

The following table summarizes the purposes of the different extra
configuration files that are available to be used with this benchmark.
A tester may mix and match them allowing them different scenarios to
be easily compared the default. Benchmark output can be saved and subsequently
exported to third party tools to compare and chart performance differences
both between configurations as well as across Zephyr versions.

+-----------------------------+------------------------------------+
| prj.canaries.conf           | Enable stack canaries              |
+-----------------------------+------------------------------------+
| prj.objcore.conf            | Enable object cores and statistics |
+-----------------------------+------------------------------------+
| prj.userspace.conf          | Enable userspace support           |
+-----------------------------+------------------------------------+

Sample output of the benchmark using the defaults::

        thread.yield.preemptive.ctx.k_to_k       - Context switch via k_yield                         :     315 cycles ,     2625 ns :
        thread.yield.cooperative.ctx.k_to_k      - Context switch via k_yield                         :     315 cycles ,     2625 ns :
        isr.resume.interrupted.thread.kernel     - Return from ISR to interrupted thread              :     289 cycles ,     2416 ns :
        isr.resume.different.thread.kernel       - Return from ISR to another thread                  :     374 cycles ,     3124 ns :
        thread.create.kernel.from.kernel         - Create thread                                      :     382 cycles ,     3191 ns :
        thread.start.kernel.from.kernel          - Start thread                                       :     394 cycles ,     3291 ns :
        thread.suspend.kernel.from.kernel        - Suspend thread                                     :     289 cycles ,     2416 ns :
        thread.resume.kernel.from.kernel         - Resume thread                                      :     339 cycles ,     2833 ns :
        thread.abort.kernel.from.kernel          - Abort thread                                       :     339 cycles ,     2833 ns :
        fifo.put.immediate.kernel                - Add data to FIFO (no ctx switch)                   :     214 cycles ,     1791 ns :
        fifo.get.immediate.kernel                - Get data from FIFO (no ctx switch)                 :     134 cycles ,     1124 ns :
        fifo.put.alloc.immediate.kernel          - Allocate to add data to FIFO (no ctx switch)       :     834 cycles ,     6950 ns :
        fifo.get.free.immediate.kernel           - Free when getting data from FIFO (no ctx switch)   :     560 cycles ,     4666 ns :
        fifo.get.blocking.k_to_k                 - Get data from FIFO (w/ ctx switch)                 :     510 cycles ,     4257 ns :
        fifo.put.wake+ctx.k_to_k                 - Add data to FIFO (w/ ctx switch)                   :     590 cycles ,     4923 ns :
        fifo.get.free.blocking.k_to_k            - Free when getting data from FIFO (w/ ctx switch)   :     510 cycles ,     4250 ns :
        fifo.put.alloc.wake+ctx.k_to_k           - Allocate to add data to FIFO (w/ ctx switch)       :     585 cycles ,     4875 ns :
        lifo.put.immediate.kernel                - Add data to LIFO (no ctx switch)                   :     214 cycles ,     1791 ns :
        lifo.get.immediate.kernel                - Get data from LIFO (no ctx switch)                 :     120 cycles ,     1008 ns :
        lifo.put.alloc.immediate.kernel          - Allocate to add data to LIFO (no ctx switch)       :     831 cycles ,     6925 ns :
        lifo.get.free.immediate.kernel           - Free when getting data from LIFO (no ctx switch)   :     555 cycles ,     4625 ns :
        lifo.get.blocking.k_to_k                 - Get data from LIFO (w/ ctx switch)                 :     502 cycles ,     4191 ns :
        lifo.put.wake+ctx.k_to_k                 - Add data to LIFO (w/ ctx switch)                   :     585 cycles ,     4875 ns :
        lifo.get.free.blocking.k_to_k            - Free when getting data from LIFO (w/ ctx switch)   :     513 cycles ,     4275 ns :
        lifo.put.alloc.wake+ctx.k_to_k           - Allocate to add data to LIFO (w/ ctx switch)       :     585 cycles ,     4881 ns :
        events.post.immediate.kernel             - Post events (nothing wakes)                        :     225 cycles ,     1875 ns :
        events.set.immediate.kernel              - Set events (nothing wakes)                         :     230 cycles ,     1923 ns :
        events.wait.immediate.kernel             - Wait for any events (no ctx switch)                :     120 cycles ,     1000 ns :
        events.wait_all.immediate.kernel         - Wait for all events (no ctx switch)                :     110 cycles ,      917 ns :
        events.wait.blocking.k_to_k              - Wait for any events (w/ ctx switch)                :     514 cycles ,     4291 ns :
        events.set.wake+ctx.k_to_k               - Set events (w/ ctx switch)                         :     754 cycles ,     6291 ns :
        events.wait_all.blocking.k_to_k          - Wait for all events (w/ ctx switch)                :     528 cycles ,     4400 ns :
        events.post.wake+ctx.k_to_k              - Post events (w/ ctx switch)                        :     765 cycles ,     6375 ns :
        semaphore.give.immediate.kernel          - Give a semaphore (no waiters)                      :      59 cycles ,      492 ns :
        semaphore.take.immediate.kernel          - Take a semaphore (no blocking)                     :      69 cycles ,      575 ns :
        semaphore.take.blocking.k_to_k           - Take a semaphore (context switch)                  :     450 cycles ,     3756 ns :
        semaphore.give.wake+ctx.k_to_k           - Give a semaphore (context switch)                  :     509 cycles ,     4249 ns :
        condvar.wait.blocking.k_to_k             - Wait for a condvar (context switch)                :     578 cycles ,     4817 ns :
        condvar.signal.wake+ctx.k_to_k           - Signal a condvar (context switch)                  :     630 cycles ,     5250 ns :
        stack.push.immediate.kernel              - Add data to k_stack (no ctx switch)                :     107 cycles ,      899 ns :
        stack.pop.immediate.kernel               - Get data from k_stack (no ctx switch)              :      80 cycles ,      674 ns :
        stack.pop.blocking.k_to_k                - Get data from k_stack (w/ ctx switch)              :     467 cycles ,     3899 ns :
        stack.push.wake+ctx.k_to_k               - Add data to k_stack (w/ ctx switch)                :     550 cycles ,     4583 ns :
        mutex.lock.immediate.recursive.kernel    - Lock a mutex                                       :      83 cycles ,      692 ns :
        mutex.unlock.immediate.recursive.kernel  - Unlock a mutex                                     :      44 cycles ,      367 ns :
        heap.malloc.immediate                    - Average time for heap malloc                       :     610 cycles ,     5083 ns :
        heap.free.immediate                      - Average time for heap free                         :     425 cycles ,     3541 ns :
        ===================================================================
        PROJECT EXECUTION SUCCESSFUL


Sample output of the benchmark with stack canaries enabled::

        thread.yield.preemptive.ctx.k_to_k       - Context switch via k_yield                         :     485 cycles ,     4042 ns :
        thread.yield.cooperative.ctx.k_to_k      - Context switch via k_yield                         :     485 cycles ,     4042 ns :
        isr.resume.interrupted.thread.kernel     - Return from ISR to interrupted thread              :     545 cycles ,     4549 ns :
        isr.resume.different.thread.kernel       - Return from ISR to another thread                  :     590 cycles ,     4924 ns :
        thread.create.kernel.from.kernel         - Create thread                                      :     585 cycles ,     4883 ns :
        thread.start.kernel.from.kernel          - Start thread                                       :     685 cycles ,     5716 ns :
        thread.suspend.kernel.from.kernel        - Suspend thread                                     :     490 cycles ,     4091 ns :
        thread.resume.kernel.from.kernel         - Resume thread                                      :     569 cycles ,     4749 ns :
        thread.abort.kernel.from.kernel          - Abort thread                                       :     629 cycles ,     5249 ns :
        fifo.put.immediate.kernel                - Add data to FIFO (no ctx switch)                   :     439 cycles ,     3666 ns :
        fifo.get.immediate.kernel                - Get data from FIFO (no ctx switch)                 :     320 cycles ,     2674 ns :
        fifo.put.alloc.immediate.kernel          - Allocate to add data to FIFO (no ctx switch)       :    1499 cycles ,    12491 ns :
        fifo.get.free.immediate.kernel           - Free when getting data from FIFO (no ctx switch)   :    1230 cycles ,    10250 ns :
        fifo.get.blocking.k_to_k                 - Get data from FIFO (w/ ctx switch)                 :     868 cycles ,     7241 ns :
        fifo.put.wake+ctx.k_to_k                 - Add data to FIFO (w/ ctx switch)                   :     991 cycles ,     8259 ns :
        fifo.get.free.blocking.k_to_k            - Free when getting data from FIFO (w/ ctx switch)   :     879 cycles ,     7325 ns :
        fifo.put.alloc.wake+ctx.k_to_k           - Allocate to add data to FIFO (w/ ctx switch)       :     990 cycles ,     8250 ns :
        lifo.put.immediate.kernel                - Add data to LIFO (no ctx switch)                   :     429 cycles ,     3582 ns :
        lifo.get.immediate.kernel                - Get data from LIFO (no ctx switch)                 :     320 cycles ,     2674 ns :
        lifo.put.alloc.immediate.kernel          - Allocate to add data to LIFO (no ctx switch)       :    1499 cycles ,    12491 ns :
        lifo.get.free.immediate.kernel           - Free when getting data from LIFO (no ctx switch)   :    1220 cycles ,    10166 ns :
        lifo.get.blocking.k_to_k                 - Get data from LIFO (w/ ctx switch)                 :     863 cycles ,     7199 ns :
        lifo.put.wake+ctx.k_to_k                 - Add data to LIFO (w/ ctx switch)                   :     985 cycles ,     8208 ns :
        lifo.get.free.blocking.k_to_k            - Free when getting data from LIFO (w/ ctx switch)   :     879 cycles ,     7325 ns :
        lifo.put.alloc.wake+ctx.k_to_k           - Allocate to add data to LIFO (w/ ctx switch)       :     985 cycles ,     8208 ns :
        events.post.immediate.kernel             - Post events (nothing wakes)                        :     420 cycles ,     3501 ns :
        events.set.immediate.kernel              - Set events (nothing wakes)                         :     420 cycles ,     3501 ns :
        events.wait.immediate.kernel             - Wait for any events (no ctx switch)                :     280 cycles ,     2334 ns :
        events.wait_all.immediate.kernel         - Wait for all events (no ctx switch)                :     270 cycles ,     2251 ns :
        events.wait.blocking.k_to_k              - Wait for any events (w/ ctx switch)                :     919 cycles ,     7665 ns :
        events.set.wake+ctx.k_to_k               - Set events (w/ ctx switch)                         :    1310 cycles ,    10924 ns :
        events.wait_all.blocking.k_to_k          - Wait for all events (w/ ctx switch)                :     954 cycles ,     7950 ns :
        events.post.wake+ctx.k_to_k              - Post events (w/ ctx switch)                        :    1340 cycles ,    11166 ns :
        semaphore.give.immediate.kernel          - Give a semaphore (no waiters)                      :     110 cycles ,      917 ns :
        semaphore.take.immediate.kernel          - Take a semaphore (no blocking)                     :     180 cycles ,     1500 ns :
        semaphore.take.blocking.k_to_k           - Take a semaphore (context switch)                  :     755 cycles ,     6292 ns :
        semaphore.give.wake+ctx.k_to_k           - Give a semaphore (context switch)                  :     812 cycles ,     6767 ns :
        condvar.wait.blocking.k_to_k             - Wait for a condvar (context switch)                :    1027 cycles ,     8558 ns :
        condvar.signal.wake+ctx.k_to_k           - Signal a condvar (context switch)                  :    1040 cycles ,     8666 ns :
        stack.push.immediate.kernel              - Add data to k_stack (no ctx switch)                :     220 cycles ,     1841 ns :
        stack.pop.immediate.kernel               - Get data from k_stack (no ctx switch)              :     205 cycles ,     1716 ns :
        stack.pop.blocking.k_to_k                - Get data from k_stack (w/ ctx switch)              :     791 cycles ,     6599 ns :
        stack.push.wake+ctx.k_to_k               - Add data to k_stack (w/ ctx switch)                :     870 cycles ,     7250 ns :
        mutex.lock.immediate.recursive.kernel    - Lock a mutex                                       :     175 cycles ,     1459 ns :
        mutex.unlock.immediate.recursive.kernel  - Unlock a mutex                                     :      61 cycles ,      510 ns :
        heap.malloc.immediate                    - Average time for heap malloc                       :    1060 cycles ,     8833 ns :
        heap.free.immediate                      - Average time for heap free                         :     899 cycles ,     7491 ns :
        ===================================================================
        PROJECT EXECUTION SUCCESSFUL

The sample output above (stack canaries are enabled) shows longer times than
for the default build. Not only does each stack frame in the call tree have
its own stack canary check, but enabling this feature impacts how the compiler
chooses to inline or not inline routines.

Sample output of the benchmark with object core enabled::

        thread.yield.preemptive.ctx.k_to_k       - Context switch via k_yield                         :     740 cycles ,     6167 ns :
        thread.yield.cooperative.ctx.k_to_k      - Context switch via k_yield                         :     740 cycles ,     6167 ns :
        isr.resume.interrupted.thread.kernel     - Return from ISR to interrupted thread              :     284 cycles ,     2374 ns :
        isr.resume.different.thread.kernel       - Return from ISR to another thread                  :     784 cycles ,     6541 ns :
        thread.create.kernel.from.kernel         - Create thread                                      :     714 cycles ,     5958 ns :
        thread.start.kernel.from.kernel          - Start thread                                       :     819 cycles ,     6833 ns :
        thread.suspend.kernel.from.kernel        - Suspend thread                                     :     704 cycles ,     5874 ns :
        thread.resume.kernel.from.kernel         - Resume thread                                      :     761 cycles ,     6349 ns :
        thread.abort.kernel.from.kernel          - Abort thread                                       :     544 cycles ,     4541 ns :
        fifo.put.immediate.kernel                - Add data to FIFO (no ctx switch)                   :     211 cycles ,     1766 ns :
        fifo.get.immediate.kernel                - Get data from FIFO (no ctx switch)                 :     132 cycles ,     1108 ns :
        fifo.put.alloc.immediate.kernel          - Allocate to add data to FIFO (no ctx switch)       :     850 cycles ,     7091 ns :
        fifo.get.free.immediate.kernel           - Free when getting data from FIFO (no ctx switch)   :     565 cycles ,     4708 ns :
        fifo.get.blocking.k_to_k                 - Get data from FIFO (w/ ctx switch)                 :     947 cycles ,     7899 ns :
        fifo.put.wake+ctx.k_to_k                 - Add data to FIFO (w/ ctx switch)                   :    1015 cycles ,     8458 ns :
        fifo.get.free.blocking.k_to_k            - Free when getting data from FIFO (w/ ctx switch)   :     950 cycles ,     7923 ns :
        fifo.put.alloc.wake+ctx.k_to_k           - Allocate to add data to FIFO (w/ ctx switch)       :    1010 cycles ,     8416 ns :
        lifo.put.immediate.kernel                - Add data to LIFO (no ctx switch)                   :     226 cycles ,     1891 ns :
        lifo.get.immediate.kernel                - Get data from LIFO (no ctx switch)                 :     123 cycles ,     1033 ns :
        lifo.put.alloc.immediate.kernel          - Allocate to add data to LIFO (no ctx switch)       :     848 cycles ,     7066 ns :
        lifo.get.free.immediate.kernel           - Free when getting data from LIFO (no ctx switch)   :     565 cycles ,     4708 ns :
        lifo.get.blocking.k_to_k                 - Get data from LIFO (w/ ctx switch)                 :     951 cycles ,     7932 ns :
        lifo.put.wake+ctx.k_to_k                 - Add data to LIFO (w/ ctx switch)                   :    1010 cycles ,     8416 ns :
        lifo.get.free.blocking.k_to_k            - Free when getting data from LIFO (w/ ctx switch)   :     959 cycles ,     7991 ns :
        lifo.put.alloc.wake+ctx.k_to_k           - Allocate to add data to LIFO (w/ ctx switch)       :    1010 cycles ,     8422 ns :
        events.post.immediate.kernel             - Post events (nothing wakes)                        :     210 cycles ,     1750 ns :
        events.set.immediate.kernel              - Set events (nothing wakes)                         :     230 cycles ,     1917 ns :
        events.wait.immediate.kernel             - Wait for any events (no ctx switch)                :     120 cycles ,     1000 ns :
        events.wait_all.immediate.kernel         - Wait for all events (no ctx switch)                :     150 cycles ,     1250 ns :
        events.wait.blocking.k_to_k              - Wait for any events (w/ ctx switch)                :     951 cycles ,     7932 ns :
        events.set.wake+ctx.k_to_k               - Set events (w/ ctx switch)                         :    1179 cycles ,     9833 ns :
        events.wait_all.blocking.k_to_k          - Wait for all events (w/ ctx switch)                :     976 cycles ,     8133 ns :
        events.post.wake+ctx.k_to_k              - Post events (w/ ctx switch)                        :    1190 cycles ,     9922 ns :
        semaphore.give.immediate.kernel          - Give a semaphore (no waiters)                      :      59 cycles ,      492 ns :
        semaphore.take.immediate.kernel          - Take a semaphore (no blocking)                     :      69 cycles ,      575 ns :
        semaphore.take.blocking.k_to_k           - Take a semaphore (context switch)                  :     870 cycles ,     7250 ns :
        semaphore.give.wake+ctx.k_to_k           - Give a semaphore (context switch)                  :     929 cycles ,     7749 ns :
        condvar.wait.blocking.k_to_k             - Wait for a condvar (context switch)                :    1010 cycles ,     8417 ns :
        condvar.signal.wake+ctx.k_to_k           - Signal a condvar (context switch)                  :    1060 cycles ,     8833 ns :
        stack.push.immediate.kernel              - Add data to k_stack (no ctx switch)                :      90 cycles ,      758 ns :
        stack.pop.immediate.kernel               - Get data from k_stack (no ctx switch)              :      86 cycles ,      724 ns :
        stack.pop.blocking.k_to_k                - Get data from k_stack (w/ ctx switch)              :     910 cycles ,     7589 ns :
        stack.push.wake+ctx.k_to_k               - Add data to k_stack (w/ ctx switch)                :     975 cycles ,     8125 ns :
        mutex.lock.immediate.recursive.kernel    - Lock a mutex                                       :     105 cycles ,      875 ns :
        mutex.unlock.immediate.recursive.kernel  - Unlock a mutex                                     :      44 cycles ,      367 ns :
        heap.malloc.immediate                    - Average time for heap malloc                       :     621 cycles ,     5183 ns :
        heap.free.immediate                      - Average time for heap free                         :     422 cycles ,     3516 ns :
        ===================================================================
        PROJECT EXECUTION SUCCESSFUL

The sample output above (object core and statistics enabled) shows longer
times than for the default build when context switching is involved. A blanket
enabling of the object cores as was done here results in the additional
gathering of thread statistics when a thread is switched in/out. The
gathering of these statistics can be controlled at both at the time of
project configuration as well as at runtime.

Sample output of the benchmark with userspace enabled::

        thread.yield.preemptive.ctx.k_to_k       - Context switch via k_yield                         :     975 cycles ,     8125 ns :
        thread.yield.preemptive.ctx.u_to_u       - Context switch via k_yield                         :    1303 cycles ,    10860 ns :
        thread.yield.preemptive.ctx.k_to_u       - Context switch via k_yield                         :    1180 cycles ,     9834 ns :
        thread.yield.preemptive.ctx.u_to_k       - Context switch via k_yield                         :    1097 cycles ,     9144 ns :
        thread.yield.cooperative.ctx.k_to_k      - Context switch via k_yield                         :     975 cycles ,     8125 ns :
        thread.yield.cooperative.ctx.u_to_u      - Context switch via k_yield                         :    1302 cycles ,    10854 ns :
        thread.yield.cooperative.ctx.k_to_u      - Context switch via k_yield                         :    1180 cycles ,     9834 ns :
        thread.yield.cooperative.ctx.u_to_k      - Context switch via k_yield                         :    1097 cycles ,     9144 ns :
        isr.resume.interrupted.thread.kernel     - Return from ISR to interrupted thread              :     329 cycles ,     2749 ns :
        isr.resume.different.thread.kernel       - Return from ISR to another thread                  :    1014 cycles ,     8457 ns :
        isr.resume.different.thread.user         - Return from ISR to another thread                  :    1223 cycles ,    10192 ns :
        thread.create.kernel.from.kernel         - Create thread                                      :     970 cycles ,     8089 ns :
        thread.start.kernel.from.kernel          - Start thread                                       :    1074 cycles ,     8957 ns :
        thread.suspend.kernel.from.kernel        - Suspend thread                                     :     949 cycles ,     7916 ns :
        thread.resume.kernel.from.kernel         - Resume thread                                      :    1004 cycles ,     8374 ns :
        thread.abort.kernel.from.kernel          - Abort thread                                       :    2734 cycles ,    22791 ns :
        thread.create.user.from.kernel           - Create thread                                      :     832 cycles ,     6935 ns :
        thread.start.user.from.kernel            - Start thread                                       :    9023 cycles ,    75192 ns :
        thread.suspend.user.from.kernel          - Suspend thread                                     :    1312 cycles ,    10935 ns :
        thread.resume.user.from.kernel           - Resume thread                                      :    1187 cycles ,     9894 ns :
        thread.abort.user.from.kernel            - Abort thread                                       :    2597 cycles ,    21644 ns :
        thread.create.user.from.user             - Create thread                                      :    2144 cycles ,    17872 ns :
        thread.start.user.from.user              - Start thread                                       :    9399 cycles ,    78330 ns :
        thread.suspend.user.from.user            - Suspend thread                                     :    1504 cycles ,    12539 ns :
        thread.resume.user.from.user             - Resume thread                                      :    1574 cycles ,    13122 ns :
        thread.abort.user.from.user              - Abort thread                                       :    3237 cycles ,    26981 ns :
        thread.start.kernel.from.user            - Start thread                                       :    1452 cycles ,    12102 ns :
        thread.suspend.kernel.from.user          - Suspend thread                                     :    1143 cycles ,     9525 ns :
        thread.resume.kernel.from.user           - Resume thread                                      :    1392 cycles ,    11602 ns :
        thread.abort.kernel.from.user            - Abort thread                                       :    3372 cycles ,    28102 ns :
        fifo.put.immediate.kernel                - Add data to FIFO (no ctx switch)                   :     239 cycles ,     1999 ns :
        fifo.get.immediate.kernel                - Get data from FIFO (no ctx switch)                 :     184 cycles ,     1541 ns :
        fifo.put.alloc.immediate.kernel          - Allocate to add data to FIFO (no ctx switch)       :     920 cycles ,     7666 ns :
        fifo.get.free.immediate.kernel           - Free when getting data from FIFO (no ctx switch)   :     650 cycles ,     5416 ns :
        fifo.put.alloc.immediate.user            - Allocate to add data to FIFO (no ctx switch)       :    1710 cycles ,    14256 ns :
        fifo.get.free.immediate.user             - Free when getting data from FIFO (no ctx switch)   :    1440 cycles ,    12000 ns :
        fifo.get.blocking.k_to_k                 - Get data from FIFO (w/ ctx switch)                 :    1209 cycles ,    10082 ns :
        fifo.put.wake+ctx.k_to_k                 - Add data to FIFO (w/ ctx switch)                   :    1230 cycles ,    10250 ns :
        fifo.get.free.blocking.k_to_k            - Free when getting data from FIFO (w/ ctx switch)   :    1210 cycles ,    10083 ns :
        fifo.put.alloc.wake+ctx.k_to_k           - Allocate to add data to FIFO (w/ ctx switch)       :    1260 cycles ,    10500 ns :
        fifo.get.free.blocking.u_to_k            - Free when getting data from FIFO (w/ ctx switch)   :    1745 cycles ,    14547 ns :
        fifo.put.alloc.wake+ctx.k_to_u           - Allocate to add data to FIFO (w/ ctx switch)       :    1600 cycles ,    13333 ns :
        fifo.get.free.blocking.k_to_u            - Free when getting data from FIFO (w/ ctx switch)   :    1550 cycles ,    12922 ns :
        fifo.put.alloc.wake+ctx.u_to_k           - Allocate to add data to FIFO (w/ ctx switch)       :    1795 cycles ,    14958 ns :
        fifo.get.free.blocking.u_to_u            - Free when getting data from FIFO (w/ ctx switch)   :    2084 cycles ,    17374 ns :
        fifo.put.alloc.wake+ctx.u_to_u           - Allocate to add data to FIFO (w/ ctx switch)       :    2135 cycles ,    17791 ns :
        lifo.put.immediate.kernel                - Add data to LIFO (no ctx switch)                   :     234 cycles ,     1957 ns :
        lifo.get.immediate.kernel                - Get data from LIFO (no ctx switch)                 :     189 cycles ,     1582 ns :
        lifo.put.alloc.immediate.kernel          - Allocate to add data to LIFO (no ctx switch)       :     935 cycles ,     7791 ns :
        lifo.get.free.immediate.kernel           - Free when getting data from LIFO (no ctx switch)   :     650 cycles ,     5416 ns :
        lifo.put.alloc.immediate.user            - Allocate to add data to LIFO (no ctx switch)       :    1715 cycles ,    14291 ns :
        lifo.get.free.immediate.user             - Free when getting data from LIFO (no ctx switch)   :    1445 cycles ,    12041 ns :
        lifo.get.blocking.k_to_k                 - Get data from LIFO (w/ ctx switch)                 :    1219 cycles ,    10166 ns :
        lifo.put.wake+ctx.k_to_k                 - Add data to LIFO (w/ ctx switch)                   :    1230 cycles ,    10250 ns :
        lifo.get.free.blocking.k_to_k            - Free when getting data from LIFO (w/ ctx switch)   :    1210 cycles ,    10083 ns :
        lifo.put.alloc.wake+ctx.k_to_k           - Allocate to add data to LIFO (w/ ctx switch)       :    1260 cycles ,    10500 ns :
        lifo.get.free.blocking.u_to_k            - Free when getting data from LIFO (w/ ctx switch)   :    1744 cycles ,    14541 ns :
        lifo.put.alloc.wake+ctx.k_to_u           - Allocate to add data to LIFO (w/ ctx switch)       :    1595 cycles ,    13291 ns :
        lifo.get.free.blocking.k_to_u            - Free when getting data from LIFO (w/ ctx switch)   :    1544 cycles ,    12874 ns :
        lifo.put.alloc.wake+ctx.u_to_k           - Allocate to add data to LIFO (w/ ctx switch)       :    1795 cycles ,    14958 ns :
        lifo.get.free.blocking.u_to_u            - Free when getting data from LIFO (w/ ctx switch)   :    2080 cycles ,    17339 ns :
        lifo.put.alloc.wake+ctx.u_to_u           - Allocate to add data to LIFO (w/ ctx switch)       :    2130 cycles ,    17750 ns :
        events.post.immediate.kernel             - Post events (nothing wakes)                        :     285 cycles ,     2375 ns :
        events.set.immediate.kernel              - Set events (nothing wakes)                         :     290 cycles ,     2417 ns :
        events.wait.immediate.kernel             - Wait for any events (no ctx switch)                :     235 cycles ,     1958 ns :
        events.wait_all.immediate.kernel         - Wait for all events (no ctx switch)                :     245 cycles ,     2042 ns :
        events.post.immediate.user               - Post events (nothing wakes)                        :     800 cycles ,     6669 ns :
        events.set.immediate.user                - Set events (nothing wakes)                         :     811 cycles ,     6759 ns :
        events.wait.immediate.user               - Wait for any events (no ctx switch)                :     780 cycles ,     6502 ns :
        events.wait_all.immediate.user           - Wait for all events (no ctx switch)                :     770 cycles ,     6419 ns :
        events.wait.blocking.k_to_k              - Wait for any events (w/ ctx switch)                :    1210 cycles ,    10089 ns :
        events.set.wake+ctx.k_to_k               - Set events (w/ ctx switch)                         :    1449 cycles ,    12082 ns :
        events.wait_all.blocking.k_to_k          - Wait for all events (w/ ctx switch)                :    1250 cycles ,    10416 ns :
        events.post.wake+ctx.k_to_k              - Post events (w/ ctx switch)                        :    1475 cycles ,    12291 ns :
        events.wait.blocking.u_to_k              - Wait for any events (w/ ctx switch)                :    1612 cycles ,    13435 ns :
        events.set.wake+ctx.k_to_u               - Set events (w/ ctx switch)                         :    1627 cycles ,    13560 ns :
        events.wait_all.blocking.u_to_k          - Wait for all events (w/ ctx switch)                :    1785 cycles ,    14875 ns :
        events.post.wake+ctx.k_to_u              - Post events (w/ ctx switch)                        :    1790 cycles ,    14923 ns :
        events.wait.blocking.k_to_u              - Wait for any events (w/ ctx switch)                :    1407 cycles ,    11727 ns :
        events.set.wake+ctx.u_to_k               - Set events (w/ ctx switch)                         :    1828 cycles ,    15234 ns :
        events.wait_all.blocking.k_to_u          - Wait for all events (w/ ctx switch)                :    1585 cycles ,    13208 ns :
        events.post.wake+ctx.u_to_k              - Post events (w/ ctx switch)                        :    2000 cycles ,    16666 ns :
        events.wait.blocking.u_to_u              - Wait for any events (w/ ctx switch)                :    1810 cycles ,    15087 ns :
        events.set.wake+ctx.u_to_u               - Set events (w/ ctx switch)                         :    2004 cycles ,    16705 ns :
        events.wait_all.blocking.u_to_u          - Wait for all events (w/ ctx switch)                :    2120 cycles ,    17666 ns :
        events.post.wake+ctx.u_to_u              - Post events (w/ ctx switch)                        :    2315 cycles ,    19291 ns :
        semaphore.give.immediate.kernel          - Give a semaphore (no waiters)                      :     125 cycles ,     1042 ns :
        semaphore.take.immediate.kernel          - Take a semaphore (no blocking)                     :     125 cycles ,     1042 ns :
        semaphore.give.immediate.user            - Give a semaphore (no waiters)                      :     645 cycles ,     5377 ns :
        semaphore.take.immediate.user            - Take a semaphore (no blocking)                     :     680 cycles ,     5669 ns :
        semaphore.take.blocking.k_to_k           - Take a semaphore (context switch)                  :    1140 cycles ,     9500 ns :
        semaphore.give.wake+ctx.k_to_k           - Give a semaphore (context switch)                  :    1174 cycles ,     9791 ns :
        semaphore.take.blocking.k_to_u           - Take a semaphore (context switch)                  :    1350 cycles ,    11251 ns :
        semaphore.give.wake+ctx.u_to_k           - Give a semaphore (context switch)                  :    1542 cycles ,    12852 ns :
        semaphore.take.blocking.u_to_k           - Take a semaphore (context switch)                  :    1512 cycles ,    12603 ns :
        semaphore.give.wake+ctx.k_to_u           - Give a semaphore (context switch)                  :    1382 cycles ,    11519 ns :
        semaphore.take.blocking.u_to_u           - Take a semaphore (context switch)                  :    1723 cycles ,    14360 ns :
        semaphore.give.wake+ctx.u_to_u           - Give a semaphore (context switch)                  :    1749 cycles ,    14580 ns :
        condvar.wait.blocking.k_to_k             - Wait for a condvar (context switch)                :    1285 cycles ,    10708 ns :
        condvar.signal.wake+ctx.k_to_k           - Signal a condvar (context switch)                  :    1315 cycles ,    10964 ns :
        condvar.wait.blocking.k_to_u             - Wait for a condvar (context switch)                :    1547 cycles ,    12898 ns :
        condvar.signal.wake+ctx.u_to_k           - Signal a condvar (context switch)                  :    1855 cycles ,    15458 ns :
        condvar.wait.blocking.u_to_k             - Wait for a condvar (context switch)                :    1990 cycles ,    16583 ns :
        condvar.signal.wake+ctx.k_to_u           - Signal a condvar (context switch)                  :    1640 cycles ,    13666 ns :
        condvar.wait.blocking.u_to_u             - Wait for a condvar (context switch)                :    2313 cycles ,    19280 ns :
        condvar.signal.wake+ctx.u_to_u           - Signal a condvar (context switch)                  :    2170 cycles ,    18083 ns :
        stack.push.immediate.kernel              - Add data to k_stack (no ctx switch)                :     189 cycles ,     1582 ns :
        stack.pop.immediate.kernel               - Get data from k_stack (no ctx switch)              :     194 cycles ,     1624 ns :
        stack.push.immediate.user                - Add data to k_stack (no ctx switch)                :     679 cycles ,     5664 ns :
        stack.pop.immediate.user                 - Get data from k_stack (no ctx switch)              :    1014 cycles ,     8455 ns :
        stack.pop.blocking.k_to_k                - Get data from k_stack (w/ ctx switch)              :    1209 cycles ,    10083 ns :
        stack.push.wake+ctx.k_to_k               - Add data to k_stack (w/ ctx switch)                :    1235 cycles ,    10291 ns :
        stack.pop.blocking.u_to_k                - Get data from k_stack (w/ ctx switch)              :    2050 cycles ,    17089 ns :
        stack.push.wake+ctx.k_to_u               - Add data to k_stack (w/ ctx switch)                :    1575 cycles ,    13125 ns :
        stack.pop.blocking.k_to_u                - Get data from k_stack (w/ ctx switch)              :    1549 cycles ,    12916 ns :
        stack.push.wake+ctx.u_to_k               - Add data to k_stack (w/ ctx switch)                :    1755 cycles ,    14625 ns :
        stack.pop.blocking.u_to_u                - Get data from k_stack (w/ ctx switch)              :    2389 cycles ,    19916 ns :
        stack.push.wake+ctx.u_to_u               - Add data to k_stack (w/ ctx switch)                :    2095 cycles ,    17458 ns :
        mutex.lock.immediate.recursive.kernel    - Lock a mutex                                       :     165 cycles ,     1375 ns :
        mutex.unlock.immediate.recursive.kernel  - Unlock a mutex                                     :      80 cycles ,      668 ns :
        mutex.lock.immediate.recursive.user      - Lock a mutex                                       :     685 cycles ,     5711 ns :
        mutex.unlock.immediate.recursive.user    - Unlock a mutex                                     :     615 cycles ,     5128 ns :
        heap.malloc.immediate                    - Average time for heap malloc                       :     626 cycles ,     5224 ns :
        heap.free.immediate                      - Average time for heap free                         :     427 cycles ,     3558 ns :
        ===================================================================
        PROJECT EXECUTION SUCCESSFUL

The sample output above (userspace enabled) shows longer times than for
the default build scenario. Enabling userspace results in additional
runtime overhead on each call to a kernel object to determine whether the
caller is in user or kernel space and consequently whether a system call
is needed or not.
