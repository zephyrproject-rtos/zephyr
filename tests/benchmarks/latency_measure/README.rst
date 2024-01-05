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
* Time it takes to add data to a FIFO/LIFO
* Time it takes to retrieve data from a FIFO/LIFO
* Time it takes to wait on a FIFO/LIFO (and context switch)
* Time it takes to wake and switch to a thread waiting on a FIFO/LIFO
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

        *** Booting Zephyr OS build zephyr-v3.5.0-3537-g5dbe0ce2622d ***
        THREAD yield.preemptive.ctx.(K -> K)                :     344 cycles ,     2866 ns :
        THREAD yield.cooperative.ctx.(K -> K)               :     344 cycles ,     2867 ns :
        ISR resume.interrupted.thread.kernel                :     498 cycles ,     4158 ns :
        ISR resume.different.thread.kernel                  :     383 cycles ,     3199 ns :
        THREAD create.kernel.from.kernel                    :     401 cycles ,     3349 ns :
        THREAD start.kernel.from.kernel                     :     418 cycles ,     3491 ns :
        THREAD suspend.kernel.from.kernel                   :     433 cycles ,     3616 ns :
        THREAD resume.kernel.from.kernel                    :     351 cycles ,     2933 ns :
        THREAD abort.kernel.from.kernel                     :     349 cycles ,     2909 ns :
        FIFO put.immediate.kernel                           :     294 cycles ,     2450 ns :
        FIFO get.immediate.kernel                           :     135 cycles ,     1133 ns :
        FIFO put.alloc.immediate.kernel                     :     906 cycles ,     7550 ns :
        FIFO get.free.immediate.kernel                      :     570 cycles ,     4750 ns :
        FIFO get.blocking.(K -> K)                          :     545 cycles ,     4542 ns :
        FIFO put.wake+ctx.(K -> K)                          :     675 cycles ,     5625 ns :
        FIFO get.free.blocking.(K -> K)                     :     555 cycles ,     4625 ns :
        FIFO put.alloc.wake+ctx.(K -> K)                    :     670 cycles ,     5583 ns :
        LIFO put.immediate.kernel                           :     282 cycles ,     2350 ns :
        LIFO get.immediate.kernel                           :     135 cycles ,     1133 ns :
        LIFO put.alloc.immediate.kernel                     :     903 cycles ,     7526 ns :
        LIFO get.free.immediate.kernel                      :     570 cycles ,     4750 ns :
        LIFO get.blocking.(K -> K)                          :     542 cycles ,     4524 ns :
        LIFO put.wake+ctx.(K -> K)                          :     670 cycles ,     5584 ns :
        LIFO get.free.blocking.(K -> K)                     :     547 cycles ,     4558 ns :
        LIFO put.alloc.wake+ctx.(K -> K)                    :     670 cycles ,     5583 ns :
        EVENTS post.immediate.kernel                        :     220 cycles ,     1833 ns :
        EVENTS set.immediate.kernel                         :     225 cycles ,     1875 ns :
        EVENTS wait.immediate.kernel                        :     125 cycles ,     1041 ns :
        EVENTS wait_all.immediate.kernel                    :     145 cycles ,     1208 ns :
        EVENTS wait.blocking.(K -> K)                       :     594 cycles ,     4958 ns :
        EVENTS set.wake+ctx.(K -> K)                        :     774 cycles ,     6451 ns :
        EVENTS wait_all.blocking.(K -> K)                   :     605 cycles ,     5042 ns :
        EVENTS post.wake+ctx.(K -> K)                       :     785 cycles ,     6542 ns :
        SEMAPHORE give.immediate.kernel                     :     165 cycles ,     1375 ns :
        SEMAPHORE take.immediate.kernel                     :      69 cycles ,      575 ns :
        SEMAPHORE take.blocking.(K -> K)                    :     489 cycles ,     4075 ns :
        SEMAPHORE give.wake+ctx.(K -> K)                    :     604 cycles ,     5033 ns :
        MUTEX lock.immediate.recursive.kernel               :     115 cycles ,      958 ns :
        MUTEX unlock.immediate.recursive.kernel             :      40 cycles ,      333 ns :
        HEAP malloc.immediate                               :     615 cycles ,     5125 ns :
        HEAP free.immediate                                 :     431 cycles ,     3591 ns :
        ===================================================================
        PROJECT EXECUTION SUCCESSFUL

Sample output of the benchmark (with userspace enabled)::

        *** Booting Zephyr OS build zephyr-v3.5.0-3537-g5dbe0ce2622d ***
        THREAD yield.preemptive.ctx.(K -> K)                :     990 cycles ,     8250 ns :
        THREAD yield.preemptive.ctx.(U -> U)                :    1285 cycles ,    10712 ns :
        THREAD yield.preemptive.ctx.(K -> U)                :    1178 cycles ,     9817 ns :
        THREAD yield.preemptive.ctx.(U -> K)                :    1097 cycles ,     9145 ns :
        THREAD yield.cooperative.ctx.(K -> K)               :     990 cycles ,     8250 ns :
        THREAD yield.cooperative.ctx.(U -> U)               :    1285 cycles ,    10712 ns :
        THREAD yield.cooperative.ctx.(K -> U)               :    1178 cycles ,     9817 ns :
        THREAD yield.cooperative.ctx.(U -> K)               :    1097 cycles ,     9146 ns :
        ISR resume.interrupted.thread.kernel                :    1120 cycles ,     9333 ns :
        ISR resume.different.thread.kernel                  :    1010 cycles ,     8417 ns :
        ISR resume.different.thread.user                    :    1207 cycles ,    10062 ns :
        THREAD create.kernel.from.kernel                    :     955 cycles ,     7958 ns :
        THREAD start.kernel.from.kernel                     :    1095 cycles ,     9126 ns :
        THREAD suspend.kernel.from.kernel                   :    1064 cycles ,     8874 ns :
        THREAD resume.kernel.from.kernel                    :     999 cycles ,     8333 ns :
        THREAD abort.kernel.from.kernel                     :    2280 cycles ,    19000 ns :
        THREAD create.user.from.kernel                      :     822 cycles ,     6855 ns :
        THREAD start.user.from.kernel                       :    6572 cycles ,    54774 ns :
        THREAD suspend.user.from.kernel                     :    1422 cycles ,    11857 ns :
        THREAD resume.user.from.kernel                      :    1177 cycles ,     9812 ns :
        THREAD abort.user.from.kernel                       :    2147 cycles ,    17897 ns :
        THREAD create.user.from.user                        :    2105 cycles ,    17542 ns :
        THREAD start.user.from.user                         :    6960 cycles ,    58002 ns :
        THREAD suspend user.from.user                       :    1610 cycles ,    13417 ns :
        THREAD resume user.from.user                        :    1565 cycles ,    13042 ns :
        THREAD abort user.from.user                         :    2780 cycles ,    23167 ns :
        THREAD start.kernel.from.user                       :    1482 cycles ,    12353 ns :
        THREAD suspend.kernel.from.user                     :    1252 cycles ,    10437 ns :
        THREAD resume.kernel.from.user                      :    1387 cycles ,    11564 ns :
        THREAD abort.kernel.from.user                       :    2912 cycles ,    24272 ns :
        FIFO put.immediate.kernel                           :     314 cycles ,     2624 ns :
        FIFO get.immediate.kernel                           :     215 cycles ,     1792 ns :
        FIFO put.alloc.immediate.kernel                     :    1025 cycles ,     8541 ns :
        FIFO get.free.immediate.kernel                      :     655 cycles ,     5458 ns :
        FIFO put.alloc.immediate.user                       :    1740 cycles ,    14500 ns :
        FIFO get.free.immediate.user                        :    1410 cycles ,    11751 ns :
        FIFO get.blocking.(K -> K)                          :    1249 cycles ,    10416 ns :
        FIFO put.wake+ctx.(K -> K)                          :    1320 cycles ,    11000 ns :
        FIFO get.free.blocking.(K -> K)                     :    1235 cycles ,    10292 ns :
        FIFO put.alloc.wake+ctx.(K -> K)                    :    1355 cycles ,    11292 ns :
        FIFO get.free.blocking.(U -> K)                     :    1750 cycles ,    14584 ns :
        FIFO put.alloc.wake+ctx.(K -> U)                    :    1680 cycles ,    14001 ns :
        FIFO get.free.blocking.(K -> U)                     :    1555 cycles ,    12959 ns :
        FIFO put.alloc.wake+ctx.(U -> K)                    :    1845 cycles ,    15375 ns :
        FIFO get.free.blocking.(U -> U)                     :    2070 cycles ,    17251 ns :
        FIFO put.alloc.wake+ctx.(U -> U)                    :    2170 cycles ,    18084 ns :
        LIFO put.immediate.kernel                           :     299 cycles ,     2499 ns :
        LIFO get.immediate.kernel                           :     204 cycles ,     1708 ns :
        LIFO put.alloc.immediate.kernel                     :    1015 cycles ,     8459 ns :
        LIFO get.free.immediate.kernel                      :     645 cycles ,     5375 ns :
        LIFO put.alloc.immediate.user                       :    1760 cycles ,    14668 ns :
        LIFO get.free.immediate.user                        :    1400 cycles ,    11667 ns :
        LIFO get.blocking.(K -> K)                          :    1234 cycles ,    10291 ns :
        LIFO put.wake+ctx.(K -> K)                          :    1315 cycles ,    10959 ns :
        LIFO get.free.blocking.(K -> K)                     :    1230 cycles ,    10251 ns :
        LIFO put.alloc.wake+ctx.(K -> K)                    :    1345 cycles ,    11208 ns :
        LIFO get.free.blocking.(U -> K)                     :    1745 cycles ,    14544 ns :
        LIFO put.alloc.wake+ctx.(K -> U)                    :    1680 cycles ,    14000 ns :
        LIFO get.free.blocking.(K -> U)                     :    1555 cycles ,    12958 ns :
        LIFO put.alloc.wake+ctx.(U -> K)                    :    1855 cycles ,    15459 ns :
        LIFO get.free.blocking.(U -> U)                     :    2070 cycles ,    17251 ns :
        LIFO put.alloc.wake+ctx.(U -> U)                    :    2190 cycles ,    18251 ns :
        EVENTS post.immediate.kernel                        :     285 cycles ,     2375 ns :
        EVENTS set.immediate.kernel                         :     285 cycles ,     2375 ns :
        EVENTS wait.immediate.kernel                        :     215 cycles ,     1791 ns :
        EVENTS wait_all.immediate.kernel                    :     215 cycles ,     1791 ns :
        EVENTS post.immediate.user                          :     775 cycles ,     6459 ns :
        EVENTS set.immediate.user                           :     780 cycles ,     6500 ns :
        EVENTS wait.immediate.user                          :     715 cycles ,     5959 ns :
        EVENTS wait_all.immediate.user                      :     720 cycles ,     6000 ns :
        EVENTS wait.blocking.(K -> K)                       :    1212 cycles ,    10108 ns :
        EVENTS set.wake+ctx.(K -> K)                        :    1450 cycles ,    12084 ns :
        EVENTS wait_all.blocking.(K -> K)                   :    1260 cycles ,    10500 ns :
        EVENTS post.wake+ctx.(K -> K)                       :    1490 cycles ,    12417 ns :
        EVENTS wait.blocking.(U -> K)                       :    1577 cycles ,    13145 ns :
        EVENTS set.wake+ctx.(K -> U)                        :    1617 cycles ,    13479 ns :
        EVENTS wait_all.blocking.(U -> K)                   :    1760 cycles ,    14667 ns :
        EVENTS post.wake+ctx.(K -> U)                       :    1790 cycles ,    14917 ns :
        EVENTS wait.blocking.(K -> U)                       :    1400 cycles ,    11671 ns :
        EVENTS set.wake+ctx.(U -> K)                        :    1812 cycles ,    15104 ns :
        EVENTS wait_all.blocking.(K -> U)                   :    1580 cycles ,    13167 ns :
        EVENTS post.wake+ctx.(U -> K)                       :    1985 cycles ,    16542 ns :
        EVENTS wait.blocking.(U -> U)                       :    1765 cycles ,    14709 ns :
        EVENTS set.wake+ctx.(U -> U)                        :    1979 cycles ,    16499 ns :
        EVENTS wait_all.blocking.(U -> U)                   :    2080 cycles ,    17334 ns :
        EVENTS post.wake+ctx.(U -> U)                       :    2285 cycles ,    19043 ns :
        SEMAPHORE give.immediate.kernel                     :     210 cycles ,     1750 ns :
        SEMAPHORE take.immediate.kernel                     :     145 cycles ,     1208 ns :
        SEMAPHORE give.immediate.user                       :     715 cycles ,     5959 ns :
        SEMAPHORE take.immediate.user                       :     660 cycles ,     5500 ns :
        SEMAPHORE take.blocking.(K -> K)                    :    1150 cycles ,     9584 ns :
        SEMAPHORE give.wake+ctx.(K -> K)                    :    1279 cycles ,    10666 ns :
        SEMAPHORE take.blocking.(K -> U)                    :    1343 cycles ,    11192 ns :
        SEMAPHORE give.wake+ctx.(U -> K)                    :    1637 cycles ,    13645 ns :
        SEMAPHORE take.blocking.(U -> K)                    :    1522 cycles ,    12688 ns :
        SEMAPHORE give.wake+ctx.(K -> U)                    :    1472 cycles ,    12270 ns :
        SEMAPHORE take.blocking.(U -> U)                    :    1715 cycles ,    14296 ns :
        SEMAPHORE give.wake+ctx.(U -> U)                    :    1830 cycles ,    15250 ns :
        MUTEX lock.immediate.recursive.kernel               :     150 cycles ,     1250 ns :
        MUTEX unlock.immediate.recursive.kernel             :      57 cycles ,      475 ns :
        MUTEX lock.immediate.recursive.user                 :     670 cycles ,     5583 ns :
        MUTEX unlock.immediate.recursive.user               :     595 cycles ,     4959 ns :
        HEAP malloc.immediate                               :     629 cycles ,     5241 ns :
        HEAP free.immediate                                 :     414 cycles ,     3450 ns :
        ===================================================================
        PROJECT EXECUTION SUCCESSFUL
