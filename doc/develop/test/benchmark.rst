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

Defining a Benchmark
=====================

A benchmark suite is defined similarly to the normal ztest testsuite by first
defining the suite with ``ZTEST_BENCHMARK_SUITE`` and then adding individual
benchmark tests using ``ZTEST_BENCHMARK`` or ``ZTEST_BENCHMARK_SETUP_TEARDOWN``
macros.

.. code-block:: c

   #include <zephyr/ztest.h>

   ZTEST_BENCHMARK_SUITE(measurements, NULL, NULL);

   ZTEST_BENCHMARK(measurements, critical_path)
   {
       /* Code to benchmark */
   }

Understanding Results
*********************

The benchmarking framework outputs a structured report to the console. This data
allows you to observe performance characteristics of the code in question.

An example output looks like this:

.. code-block:: console

   <suite name> ###############################################
   <benchmark name> ===========================================
      Sample size:<number of samples>, total cycles: <total amount of cycles>
      Mean(u): <mean cycles per iteration>
      Standard deviation(s): <cycles>
      Standard Error(SE): <cycles>
      Min: <cycles> (run #<iteration number>)
      Max: <cycles> (run #<iteration number>)


Statistical Metrics
===================

* **Mean (u)**: The average number of cycles taken per iteration. It provides a
  central value representing the expected cost of execution.

* **Standard Deviation (s)**: Measures the amount of variation or fluctuation of
  the execution cost from the mean. A low standard deviation indicates that the
  behavior is deterministic and consistent.

* **Standard Error (SE)**: Estimates how far the sample mean is likely to be
  from the "true" mean of the system. It provides insight into the statistical
  reliability of the test. A lower SE indicates higher confidence in the
  result.

* **Min/Max**: The minimum and maximum cycle counts observed, along with which
  iteration they occurred on.

Plainly speaking, lower values are better for all metrics.
Lower mean, min, and max values indicates better raw performance.
Lower standard deviation and standard error values indicate more consistent and
therefore reliable performance.

.. note::
   An impressive benchmark result with a low sample size is *not* helping anyone.
   Always aim for a sufficiently large number of iterations to get statistically
   significant results. Choose your sample size wisely.

Important Considerations
************************

* **Noise**: Benchmarking is inherently sensitive to system noise. To obtain as
  accurate results as possible, disable unnecessary background tasks and
  interrupts that may interfere with timing.

* **Cache Warming**: The first iteration of a benchmark may often be slower
  due to cache misses and this can skew results slightly on small sample sizes,
  choose a sufficiently large number of iterations to mitigate this effect.

* **Use setup/teardown functions**: It is *highly* encouraged to utilize the
  setup and teardown functions to isolate the code under test as much as
  possible. Excessive setup/teardown code in the benchmark code can introduce
  noise and skew results *significantly* for small critical paths.


API Reference
*************

.. doxygengroup:: ztest_benchmark
