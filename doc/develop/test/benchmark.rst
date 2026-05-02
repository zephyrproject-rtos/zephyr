.. _ztest_benchmarking:

Benchmarking Framework
######################

The Zephyr benchmarking framework provides cycle-accurate performance
measurements. It automates data collection and statistical calculation, offering
a standardized way to evaluate execution metrics across the Zephyr ecosystem.

Overview
********

This framework helps identify regressions and optimize critical paths by
providing:

* **Standardized API**: Macros that align with existing ``ztest`` conventions.
* **Statistical Analysis**: Calculations of Mean, Standard Deviation, Standard
  Error, and Min/Max values.
* **Overhead Compensation**: Inclusion of a control test to account for the
  benchmarking frameworks own execution time.

Configuration
*************

To use the benchmarking framework, you must enable the following Kconfig
options:

.. code-block:: cfg

   CONFIG_ZTEST=y
   CONFIG_ZTEST_BENCHMARK=y

Usage
*****

A benchmark suite is defined similarly to the normal ztest testsuite by first
defining the suite with ``ZTEST_BENCHMARK_SUITE`` and then adding individual
benchmark tests to the suite using either ``ZTEST_BENCHMARK`` or ``ZTEST_BENCHMARK_TIMED`` macros.

.. code-block:: c

   #include <zephyr/ztest.h>

   ZTEST_BENCHMARK_SUITE(<test suite name>, <setup_fn>, <teardown_fn>);


Standard Benchmarks
===================

Standard benchmarks are sample-based, meaning they execute the test a specified number
of times and measure the total cycles taken. This is useful for benchmarking critical paths where
you want to understand the raw CPU performance in terms of cycles. It provides insights into the
efficiency of the code and helps identify bottlenecks in terms of CPU usage.
This benchmarking method is suitable for code that has consistent execution times and is not heavily
influenced by external factors such as I/O operations or context switches.

.. code-block:: c

   #include <zephyr/ztest.h>

   ZTEST_BENCHMARK_SUITE(<suite name>, NULL, NULL);

   ZTEST_BENCHMARK(<suite name>, <benchmark name>, <number of samples>)
   {
       /* Code to benchmark */
   }

A standard benchmark follows a flow where the setup function is called before each sample, the test
function is executed for the specified number of samples, and the teardown function is called after
each sample.

Timed Benchmarks
================

Timed benchmarks in contrast to the standard benchmarks measures execution time of the code
instead of cycles. This is useful for benchmarking code that may have variable execution
times or when you want to measure the actual time taken rather than just CPU cycles of a critical
path. It provides a broader view of performance characteristics, especially for code that involves
I/O operations, context switches, or other factors that can influence execution time beyond raw CPU
performance.

.. code-block:: c

   ZTEST_BENCHMARK_TIMED(<suite name>, <benchmark name>, <time in milliseconds>)
   {
         /* Code to benchmark */
   }


Unlike standard benchmarks that favor isolation, Timed Benchmarks executes setup and teardown
functions only once allowing the test function to run hot within a dedicated time window.
This will give a more realistic measurements as it includes the overhead of the system as it would
be in a real-world scenario, such as interrupts, context switches, and other background tasks.

Understanding Results
*********************

Standard Benchmarking Results
=============================

.. code-block:: console

   <suite name> ###############################################
   <benchmark name> ===========================================
      Sample size:<number of samples>, total cycles: <total amount of cycles>
      Mean(u): <mean cycles per sample>
      Standard deviation(s): <cycles>
      Standard Error(SE): <cycles>
      Min: <cycles> (run #<sample number>)
      Max: <cycles> (run #<sample number>)


Statistical Metrics
"""""""""""""""""""

* **Mean (u)**: The average number of cycles taken per sample. It provides a
  central value representing the expected cost of execution.

* **Standard Deviation (s)**: Measures the amount of variation or fluctuation of
  the execution cost from the mean. A low standard deviation indicates that the
  behavior is deterministic and consistent.

* **Standard Error (SE)**: Estimates how far the sample mean is likely to be
  from the "true" mean of the system. It provides insight into the statistical
  reliability of the test. A lower SE indicates higher confidence in the
  result.

* **Min/Max**: The minimum and maximum cycle counts observed, along with which
  sample they occurred on.

Plainly speaking, lower values are better for all metrics.
Lower mean, min, and max values indicates better raw performance.
Lower standard deviation and standard error values indicate more consistent and
therefore reliable performance.

Timed Benchmarking Results
==========================

.. code-block:: console

   <benchmark name> ===============================================
      Samples: <Number of samples executed during the benchmark>
      Total Time: <Gross execution time>
      Work Time: <Net execution time> ns (Net)
      Ops/Sec: <average operations per second>
      Cycles/Op: <average cycles per operation>

Statistical Metrics
"""""""""""""""""""
* **Total Time**: The total time taken for all samples of the benchmark, including overhead.

* **Work Time**: The total time taken for the code under test, excluding the overhead of the
  benchmarking framework itself. This provides a more accurate measure of the actual performance of
  the code being benchmarked.

* **Ops/Sec**: The number of operations (samples) that can be performed per second, calculated by
  dividing the number of samples by the net work time (in seconds). This metric is particularly
  useful for understanding the throughput of the code being benchmarked.

* **Cycles/Op**: The average number of CPU cycles taken per operation, calculated by dividing the
  net number of cycles by the number of samples. This metric provides insight into the
  efficiency of the code in terms of CPU usage.

In general higher values for Ops/Sec are better as it indicates higher throughput, just as lower
values for Cycles/Op are better. As the Cycles/Op and Ops/Sec metrics are derived from the same
underlying data, they are reflecting the same performance characteristics from different
perspectives. A high Ops/Sec should correspond to a low Cycles/Op, and vice versa.


Benchmark Output Options
========================

The benchmarking framework provides several options for outputting results of benchmarking data. By
default, results are printed in a verbose human-readable format that is easy to understand and
interpret.
Alternatively, you can enable the :kconfig:option:`CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV` Kconfig option
to output results in a CSV format that can be easily imported by scripts for further analysis.
The CSV output includes all the same metrics as the verbose output, but in a
format that is more conducive to automated analysis and reporting.

The standard benchmark csv output format is as follows:

.. code-block:: console

   S,<suite name>,<benchmark name>,<sample size>,<total cycles>,<mean>,<stddev>,<stderr>,<min>,<min sample>,<max>,<max sample>

The timed benchmark csv output format is as follows:

.. code-block:: console

   T,<suite name>,<benchmark name>,<samples>,<total time>,<work time>,<ops/sec>,<cycles/op>


Important Considerations
************************

* **Noise**: Benchmarking is inherently sensitive to system noise. To obtain as
  accurate results as possible, disable unnecessary background tasks and
  interrupts that may interfere with timing.

* **Cache Warming**: The first sample of a benchmark may often be slower
  due to cache misses and this can skew results slightly on small sample sizes,
  choose a sufficiently large number of samples to mitigate this effect.

* **Use setup/teardown functions**: It is *highly* encouraged to utilize the
  setup and teardown functions to isolate the code under test as much as
  possible. Excessive setup/teardown code in the benchmark code can introduce
  noise and skew results *significantly* for small critical paths.


API Reference
*************

.. doxygengroup:: ztest_benchmark
