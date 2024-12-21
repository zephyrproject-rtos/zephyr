POSIX Thread Benchmark
######################

Overview
********

This benchmark creates and joins as many threads as possible within a configurable time window.
It provides a rough comparison Zephyr's POSIX threads (pthreads) with Zephyr's kernel threads
(k_threads) API, highlighting the overhead of the POSIX. Ideally, this overhead would shrink over
time.

Sample output of the benchmark::

    *** Booting Zephyr OS build v4.0.0-1410-gfca33facee37 ***
    ASSERT: y
    BOARD: qemu_riscv64
    NUM_CPUS: 1
    TEST_DELAY_US: 0
    TEST_DURATION_S: 5
    SMP: n
    API, Thread ID, time(s), threads, cores, rate (threads/s/core)
    k_thread, ALL, 5, 47663, 1, 9532
    pthread, ALL, 5, 28180, 1, 5636
    PROJECT EXECUTION SUCCESSFUL

To observe periodic statistics on a per-thread basis in addition to the summary of statistics
printed at the end of execution, use CONFIG_TEST_PERIODIC_STATS.

Several other options can be tuned on an as-needed basis:

- CONFIG_MP_MAX_NUM_CPUS - Number of CPUs to use in parallel.
- CONFIG_TEST_DURATION_S - Number of seconds to run the test.
- CONFIG_TEST_DELAY_US - Microseconds to delay between pthread join and create.
- CONFIG_TEST_KTHREADS - Exercise k_threads in the test app.
- CONFIG_TEST_PTHREADS - Exercise pthreads in the test app.
- CONFIG_TEST_STACK_SIZE - Size of each thread stack in this test.

The following table summarizes the purposes of the different extra
configuration files that are available to be used with this benchmark.
A tester may mix and match them allowing them different scenarios to
be easily compared the default.

+-----------------------------+----------------------------------------+
| prj-assert.conf             | Enable assertions for API verification |
+-----------------------------+----------------------------------------+
