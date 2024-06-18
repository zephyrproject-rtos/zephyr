Signal Benchmark
################

Overview
********

This benchmark spawns two threads that signal one another in a tight loop for a specified duration.
The benchmark measures the number of signals sent per second per core and prints the results at
the end of execution

Sample output of the benchmark::

    *** Booting Zephyr OS build v4.3.0-1-g1c2941edff59 ***
    USERSPACE: y
    ASSERT: y
    BOARD: qemu_riscv64
    NUM_CPUS: 1
    TEST_DURATION_S: 5
    SMP: n
    duration, num_cpus, retries, signals, retries/s/cpu, signals/s/cpu
    5, 1, 0, 21259, 0, 4251
    PROJECT EXECUTION SUCCESSFUL

Several other options can be tuned on an as-needed basis:

- CONFIG_MP_MAX_NUM_CPUS - Number of CPUs to use in parallel.
- CONFIG_TEST_DURATION_S - Number of seconds to run the test.
- CONFIG_TEST_STACK_SIZE - Size of each thread stack in this test.

The following table summarizes the purposes of the different extra
configuration files that are available to be used with this benchmark.
A tester may mix and match them allowing them different scenarios to
be easily compared the default.

+-----------------------------+----------------------------------------+
| prj-assert.conf             | Enable assertions for API verification |
+-----------------------------+----------------------------------------+
