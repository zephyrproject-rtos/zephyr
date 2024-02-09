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
* Measure average time to alloc memory from heap then free that memory

When userspace is enabled using the prj_user.conf configuration file, this benchmark will
where possible, also test the above capabilities using various configurations involving user
threads:

* Kernel thread to kernel thread
* Kernel thread to user thread
* User thread to kernel thread
* User thread to user thread

Sample output of the benchmark (without userspace enabled)::

        *** Booting Zephyr OS build zephyr-v3.5.0-4267-g6ccdc31233a3 ***
        thread.yield.preemptive.ctx.k_to_k       - Context switch via k_yield                         :     329 cycles ,     2741 ns :
        thread.yield.cooperative.ctx.k_to_k      - Context switch via k_yield                         :     329 cycles ,     2741 ns :
        isr.resume.interrupted.thread.kernel     - Return from ISR to interrupted thread              :     363 cycles ,     3033 ns :
        isr.resume.different.thread.kernel       - Return from ISR to another thread                  :     404 cycles ,     3367 ns :
        thread.create.kernel.from.kernel         - Create thread                                      :     404 cycles ,     3374 ns :
        thread.start.kernel.from.kernel          - Start thread                                       :     423 cycles ,     3533 ns :
        thread.suspend.kernel.from.kernel        - Suspend thread                                     :     428 cycles ,     3574 ns :
        thread.resume.kernel.from.kernel         - Resume thread                                      :     350 cycles ,     2924 ns :
        thread.abort.kernel.from.kernel          - Abort thread                                       :     339 cycles ,     2826 ns :
        fifo.put.immediate.kernel                - Add data to FIFO (no ctx switch)                   :     269 cycles ,     2242 ns :
        fifo.get.immediate.kernel                - Get data from FIFO (no ctx switch)                 :     128 cycles ,     1074 ns :
        fifo.put.alloc.immediate.kernel          - Allocate to add data to FIFO (no ctx switch)       :     945 cycles ,     7875 ns :
        fifo.get.free.immediate.kernel           - Free when getting data from FIFO (no ctx switch)   :     575 cycles ,     4792 ns :
        fifo.get.blocking.k_to_k                 - Get data from FIFO (w/ ctx switch)                 :     551 cycles ,     4592 ns :
        fifo.put.wake+ctx.k_to_k                 - Add data to FIFO (w/ ctx switch)                   :     660 cycles ,     5500 ns :
        fifo.get.free.blocking.k_to_k            - Free when getting data from FIFO (w/ ctx siwtch)   :     553 cycles ,     4608 ns :
        fifo.put.alloc.wake+ctx.k_to_k           - Allocate to add data to FIFO (w/ ctx switch)       :     655 cycles ,     5458 ns :
        lifo.put.immediate.kernel                - Add data to LIFO (no ctx switch)                   :     280 cycles ,     2341 ns :
        lifo.get.immediate.kernel                - Get data from LIFO (no ctx switch)                 :     133 cycles ,     1116 ns :
        lifo.put.alloc.immediate.kernel          - Allocate to add data to LIFO (no ctx switch)       :     945 cycles ,     7875 ns :
        lifo.get.free.immediate.kernel           - Free when getting data from LIFO (no ctx switch)   :     580 cycles ,     4833 ns :
        lifo.get.blocking.k_to_k                 - Get data from LIFO (w/ ctx switch)                 :     553 cycles ,     4608 ns :
        lifo.put.wake+ctx.k_to_k                 - Add data to LIFO (w/ ctx switch)                   :     655 cycles ,     5458 ns :
        lifo.get.free.blocking.k_to_k            - Free when getting data from LIFO (w/ ctx switch)   :     550 cycles ,     4583 ns :
        lifo.put.alloc.wake+ctx.k_to_k           - Allocate to add data to LIFO (w/ ctx siwtch)       :     655 cycles ,     5458 ns :
        events.post.immediate.kernel             - Post events (nothing wakes)                        :     225 cycles ,     1875 ns :
        events.set.immediate.kernel              - Set events (nothing wakes)                         :     225 cycles ,     1875 ns :
        events.wait.immediate.kernel             - Wait for any events (no ctx switch)                :     130 cycles ,     1083 ns :
        events.wait_all.immediate.kernel         - Wait for all events (no ctx switch)                :     135 cycles ,     1125 ns :
        events.wait.blocking.k_to_k              - Wait for any events (w/ ctx switch)                :     573 cycles ,     4783 ns :
        events.set.wake+ctx.k_to_k               - Set events (w/ ctx switch)                         :     784 cycles ,     6534 ns :
        events.wait_all.blocking.k_to_k          - Wait for all events (w/ ctx switch)                :     589 cycles ,     4916 ns :
        events.post.wake+ctx.k_to_k              - Post events (w/ ctx switch)                        :     795 cycles ,     6626 ns :
        semaphore.give.immediate.kernel          - Give a semaphore (no waiters)                      :     125 cycles ,     1041 ns :
        semaphore.take.immediate.kernel          - Take a semaphore (no blocking)                     :      69 cycles ,      575 ns :
        semaphore.take.blocking.k_to_k           - Take a semaphore (context switch)                  :     494 cycles ,     4116 ns :
        semaphore.give.wake+ctx.k_to_k           - Give a semaphore (context switch)                  :     599 cycles ,     4992 ns :
        condvar.wait.blocking.k_to_k             - Wait for a condvar (context switch)                :     692 cycles ,     5767 ns :
        condvar.signal.wake+ctx.k_to_k           - Signal a condvar (context switch)                  :     715 cycles ,     5958 ns :
        mutex.lock.immediate.recursive.kernel    - Lock a mutex                                       :     100 cycles ,      833 ns :
        mutex.unlock.immediate.recursive.kernel  - Unlock a mutex                                     :      40 cycles ,      333 ns :
        heap.malloc.immediate                    - Average time for heap malloc                       :     627 cycles ,     5225 ns :
        heap.free.immediate                      - Average time for heap free                         :     432 cycles ,     3600 ns :
        ===================================================================
        PROJECT EXECUTION SUCCESSFUL


Sample output of the benchmark (with userspace enabled)::

        *** Booting Zephyr OS build zephyr-v3.5.0-4268-g6af7a1230a08 ***
        thread.yield.preemptive.ctx.k_to_k       - Context switch via k_yield                         :     970 cycles ,     8083 ns :
        thread.yield.preemptive.ctx.u_to_u       - Context switch via k_yield                         :    1260 cycles ,    10506 ns :
        thread.yield.preemptive.ctx.k_to_u       - Context switch via k_yield                         :    1155 cycles ,     9632 ns :
        thread.yield.preemptive.ctx.u_to_k       - Context switch via k_yield                         :    1075 cycles ,     8959 ns :
        thread.yield.cooperative.ctx.k_to_k      - Context switch via k_yield                         :     970 cycles ,     8083 ns :
        thread.yield.cooperative.ctx.u_to_u      - Context switch via k_yield                         :    1260 cycles ,    10506 ns :
        thread.yield.cooperative.ctx.k_to_u      - Context switch via k_yield                         :    1155 cycles ,     9631 ns :
        thread.yield.cooperative.ctx.u_to_k      - Context switch via k_yield                         :    1075 cycles ,     8959 ns :
        isr.resume.interrupted.thread.kernel     - Return from ISR to interrupted thread              :     415 cycles ,     3458 ns :
        isr.resume.different.thread.kernel       - Return from ISR to another thread                  :     985 cycles ,     8208 ns :
        isr.resume.different.thread.user         - Return from ISR to another thread                  :    1180 cycles ,     9833 ns :
        thread.create.kernel.from.kernel         - Create thread                                      :     989 cycles ,     8249 ns :
        thread.start.kernel.from.kernel          - Start thread                                       :    1059 cycles ,     8833 ns :
        thread.suspend.kernel.from.kernel        - Suspend thread                                     :    1030 cycles ,     8583 ns :
        thread.resume.kernel.from.kernel         - Resume thread                                      :     994 cycles ,     8291 ns :
        thread.abort.kernel.from.kernel          - Abort thread                                       :    2370 cycles ,    19751 ns :
        thread.create.user.from.kernel           - Create thread                                      :     860 cycles ,     7167 ns :
        thread.start.user.from.kernel            - Start thread                                       :    8965 cycles ,    74713 ns :
        thread.suspend.user.from.kernel          - Suspend thread                                     :    1400 cycles ,    11666 ns :
        thread.resume.user.from.kernel           - Resume thread                                      :    1174 cycles ,     9791 ns :
        thread.abort.user.from.kernel            - Abort thread                                       :    2240 cycles ,    18666 ns :
        thread.create.user.from.user             - Create thread                                      :    2105 cycles ,    17542 ns :
        thread.start.user.from.user              - Start thread                                       :    9345 cycles ,    77878 ns :
        thread.suspend.user.from.user            - Suspend thread                                     :    1590 cycles ,    13250 ns :
        thread.resume.user.from.user             - Resume thread                                      :    1534 cycles ,    12791 ns :
        thread.abort.user.from.user              - Abort thread                                       :    2850 cycles ,    23750 ns :
        thread.start.kernel.from.user            - Start thread                                       :    1440 cycles ,    12000 ns :
        thread.suspend.kernel.from.user          - Suspend thread                                     :    1219 cycles ,    10166 ns :
        thread.resume.kernel.from.user           - Resume thread                                      :    1355 cycles ,    11292 ns :
        thread.abort.kernel.from.user            - Abort thread                                       :    2980 cycles ,    24834 ns :
        fifo.put.immediate.kernel                - Add data to FIFO (no ctx switch)                   :     315 cycles ,     2625 ns :
        fifo.get.immediate.kernel                - Get data from FIFO (no ctx switch)                 :     209 cycles ,     1749 ns :
        fifo.put.alloc.immediate.kernel          - Allocate to add data to FIFO (no ctx switch)       :    1040 cycles ,     8667 ns :
        fifo.get.free.immediate.kernel           - Free when getting data from FIFO (no ctx switch)   :     670 cycles ,     5583 ns :
        fifo.put.alloc.immediate.user            - Allocate to add data to FIFO (no ctx switch)       :    1765 cycles ,    14709 ns :
        fifo.get.free.immediate.user             - Free when getting data from FIFO (no ctx switch)   :    1410 cycles ,    11750 ns :
        fifo.get.blocking.k_to_k                 - Get data from FIFO (w/ ctx switch)                 :    1220 cycles ,    10168 ns :
        fifo.put.wake+ctx.k_to_k                 - Add data to FIFO (w/ ctx switch)                   :    1285 cycles ,    10708 ns :
        fifo.get.free.blocking.k_to_k            - Free when getting data from FIFO (w/ ctx siwtch)   :    1235 cycles ,    10291 ns :
        fifo.put.alloc.wake+ctx.k_to_k           - Allocate to add data to FIFO (w/ ctx switch)       :    1340 cycles ,    11167 ns :
        fifo.get.free.blocking.u_to_k            - Free when getting data from FIFO (w/ ctx siwtch)   :    1715 cycles ,    14292 ns :
        fifo.put.alloc.wake+ctx.k_to_u           - Allocate to add data to FIFO (w/ ctx switch)       :    1665 cycles ,    13876 ns :
        fifo.get.free.blocking.k_to_u            - Free when getting data from FIFO (w/ ctx siwtch)   :    1565 cycles ,    13042 ns :
        fifo.put.alloc.wake+ctx.u_to_k           - Allocate to add data to FIFO (w/ ctx switch)       :    1815 cycles ,    15126 ns :
        fifo.get.free.blocking.u_to_u            - Free when getting data from FIFO (w/ ctx siwtch)   :    2045 cycles ,    17042 ns :
        fifo.put.alloc.wake+ctx.u_to_u           - Allocate to add data to FIFO (w/ ctx switch)       :    2140 cycles ,    17834 ns :
        lifo.put.immediate.kernel                - Add data to LIFO (no ctx switch)                   :     309 cycles ,     2583 ns :
        lifo.get.immediate.kernel                - Get data from LIFO (no ctx switch)                 :     219 cycles ,     1833 ns :
        lifo.put.alloc.immediate.kernel          - Allocate to add data to LIFO (no ctx switch)       :    1030 cycles ,     8583 ns :
        lifo.get.free.immediate.kernel           - Free when getting data from LIFO (no ctx switch)   :     685 cycles ,     5708 ns :
        lifo.put.alloc.immediate.user            - Allocate to add data to LIFO (no ctx switch)       :    1755 cycles ,    14625 ns :
        lifo.get.free.immediate.user             - Free when getting data from LIFO (no ctx switch)   :    1405 cycles ,    11709 ns :
        lifo.get.blocking.k_to_k                 - Get data from LIFO (w/ ctx switch)                 :    1229 cycles ,    10249 ns :
        lifo.put.wake+ctx.k_to_k                 - Add data to LIFO (w/ ctx switch)                   :    1290 cycles ,    10751 ns :
        lifo.get.free.blocking.k_to_k            - Free when getting data from LIFO (w/ ctx switch)   :    1235 cycles ,    10292 ns :
        lifo.put.alloc.wake+ctx.k_to_k           - Allocate to add data to LIFO (w/ ctx siwtch)       :    1310 cycles ,    10917 ns :
        lifo.get.free.blocking.u_to_k            - Free when getting data from LIFO (w/ ctx switch)   :    1715 cycles ,    14293 ns :
        lifo.put.alloc.wake+ctx.k_to_u           - Allocate to add data to LIFO (w/ ctx siwtch)       :    1630 cycles ,    13583 ns :
        lifo.get.free.blocking.k_to_u            - Free when getting data from LIFO (w/ ctx switch)   :    1554 cycles ,    12958 ns :
        lifo.put.alloc.wake+ctx.u_to_k           - Allocate to add data to LIFO (w/ ctx siwtch)       :    1805 cycles ,    15043 ns :
        lifo.get.free.blocking.u_to_u            - Free when getting data from LIFO (w/ ctx switch)   :    2035 cycles ,    16959 ns :
        lifo.put.alloc.wake+ctx.u_to_u           - Allocate to add data to LIFO (w/ ctx siwtch)       :    2125 cycles ,    17709 ns :
        events.post.immediate.kernel             - Post events (nothing wakes)                        :     295 cycles ,     2458 ns :
        events.set.immediate.kernel              - Set events (nothing wakes)                         :     300 cycles ,     2500 ns :
        events.wait.immediate.kernel             - Wait for any events (no ctx switch)                :     220 cycles ,     1833 ns :
        events.wait_all.immediate.kernel         - Wait for all events (no ctx switch)                :     215 cycles ,     1791 ns :
        events.post.immediate.user               - Post events (nothing wakes)                        :     795 cycles ,     6625 ns :
        events.set.immediate.user                - Set events (nothing wakes)                         :     790 cycles ,     6584 ns :
        events.wait.immediate.user               - Wait for any events (no ctx switch)                :     740 cycles ,     6167 ns :
        events.wait_all.immediate.user           - Wait for all events (no ctx switch)                :     740 cycles ,     6166 ns :
        events.wait.blocking.k_to_k              - Wait for any events (w/ ctx switch)                :    1190 cycles ,     9918 ns :
        events.set.wake+ctx.k_to_k               - Set events (w/ ctx switch)                         :    1464 cycles ,    12208 ns :
        events.wait_all.blocking.k_to_k          - Wait for all events (w/ ctx switch)                :    1235 cycles ,    10292 ns :
        events.post.wake+ctx.k_to_k              - Post events (w/ ctx switch)                        :    1500 cycles ,    12500 ns :
        events.wait.blocking.u_to_k              - Wait for any events (w/ ctx switch)                :    1580 cycles ,    13167 ns :
        events.set.wake+ctx.k_to_u               - Set events (w/ ctx switch)                         :    1630 cycles ,    13583 ns :
        events.wait_all.blocking.u_to_k          - Wait for all events (w/ ctx switch)                :    1765 cycles ,    14708 ns :
        events.post.wake+ctx.k_to_u              - Post events (w/ ctx switch)                        :    1795 cycles ,    14960 ns :
        events.wait.blocking.k_to_u              - Wait for any events (w/ ctx switch)                :    1375 cycles ,    11459 ns :
        events.set.wake+ctx.u_to_k               - Set events (w/ ctx switch)                         :    1825 cycles ,    15209 ns :
        events.wait_all.blocking.k_to_u          - Wait for all events (w/ ctx switch)                :    1555 cycles ,    12958 ns :
        events.post.wake+ctx.u_to_k              - Post events (w/ ctx switch)                        :    1995 cycles ,    16625 ns :
        events.wait.blocking.u_to_u              - Wait for any events (w/ ctx switch)                :    1765 cycles ,    14708 ns :
        events.set.wake+ctx.u_to_u               - Set events (w/ ctx switch)                         :    1989 cycles ,    16583 ns :
        events.wait_all.blocking.u_to_u          - Wait for all events (w/ ctx switch)                :    2085 cycles ,    17376 ns :
        events.post.wake+ctx.u_to_u              - Post events (w/ ctx switch)                        :    2290 cycles ,    19084 ns :
        semaphore.give.immediate.kernel          - Give a semaphore (no waiters)                      :     220 cycles ,     1833 ns :
        semaphore.take.immediate.kernel          - Take a semaphore (no blocking)                     :     130 cycles ,     1083 ns :
        semaphore.give.immediate.user            - Give a semaphore (no waiters)                      :     710 cycles ,     5917 ns :
        semaphore.take.immediate.user            - Take a semaphore (no blocking)                     :     655 cycles ,     5458 ns :
        semaphore.take.blocking.k_to_k           - Take a semaphore (context switch)                  :    1135 cycles ,     9458 ns :
        semaphore.give.wake+ctx.k_to_k           - Give a semaphore (context switch)                  :    1244 cycles ,    10374 ns :
        semaphore.take.blocking.k_to_u           - Take a semaphore (context switch)                  :    1325 cycles ,    11048 ns :
        semaphore.give.wake+ctx.u_to_k           - Give a semaphore (context switch)                  :    1610 cycles ,    13416 ns :
        semaphore.take.blocking.u_to_k           - Take a semaphore (context switch)                  :    1499 cycles ,    12499 ns :
        semaphore.give.wake+ctx.k_to_u           - Give a semaphore (context switch)                  :    1434 cycles ,    11957 ns :
        semaphore.take.blocking.u_to_u           - Take a semaphore (context switch)                  :    1690 cycles ,    14090 ns :
        semaphore.give.wake+ctx.u_to_u           - Give a semaphore (context switch)                  :    1800 cycles ,    15000 ns :
        condvar.wait.blocking.k_to_k             - Wait for a condvar (context switch)                :    1385 cycles ,    11542 ns :
        condvar.signal.wake+ctx.k_to_k           - Signal a condvar (context switch)                  :    1420 cycles ,    11833 ns :
        condvar.wait.blocking.k_to_u             - Wait for a condvar (context switch)                :    1537 cycles ,    12815 ns :
        condvar.signal.wake+ctx.u_to_k           - Signal a condvar (context switch)                  :    1950 cycles ,    16250 ns :
        condvar.wait.blocking.u_to_k             - Wait for a condvar (context switch)                :    2025 cycles ,    16875 ns :
        condvar.signal.wake+ctx.k_to_u           - Signal a condvar (context switch)                  :    1715 cycles ,    14298 ns :
        condvar.wait.blocking.u_to_u             - Wait for a condvar (context switch)                :    2313 cycles ,    19279 ns :
        condvar.signal.wake+ctx.u_to_u           - Signal a condvar (context switch)                  :    2225 cycles ,    18541 ns :
        mutex.lock.immediate.recursive.kernel    - Lock a mutex                                       :     155 cycles ,     1291 ns :
        mutex.unlock.immediate.recursive.kernel  - Unlock a mutex                                     :      57 cycles ,      475 ns :
        mutex.lock.immediate.recursive.user      - Lock a mutex                                       :     665 cycles ,     5541 ns :
        mutex.unlock.immediate.recursive.user    - Unlock a mutex                                     :     585 cycles ,     4875 ns :
        heap.malloc.immediate                    - Average time for heap malloc                       :     640 cycles ,     5341 ns :
        heap.free.immediate                      - Average time for heap free                         :     436 cycles ,     3633 ns :
        ===================================================================
        PROJECT EXECUTION SUCCESSFUL
