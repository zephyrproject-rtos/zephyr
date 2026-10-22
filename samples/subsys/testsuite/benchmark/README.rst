.. zephyr:code-sample:: ztest_benchmark
   :name: zephyr benchmark sample

   Showcases how to use Zephyr Benchmarking subsystem to measure
   performance of code sections.

Overview
********

This sample demonstrates the usage of Zephyr Benchmarking subsystem
to measure performance of code blocks reliably.

Sample Output
=============

This is the sample output when running the sample application on qemu_x86:

.. code-block:: console

   math_benchmarks ############################################
   [00:00:00.160,000] <dbg> test_benchmarks: suit_setup: Setting up math benchmarks suite
   benchmark_addition =========================================
      Sample size:100, total cycles: 28800
      Mean(μ̂): 288.000
      Standard deviation(s): 0.000
      Standard Error(SE): 0.000
      Min: 288 (run #1)
      Max: 288 (run #1)
   benchmark_pure_asm_add =====================================
      Sample size:100, total cycles: 3200
      Mean(μ̂): 32.000
      Standard deviation(s): 0.000
      Standard Error(SE): 0.000
      Min: 32 (run #1)
      Max: 32 (run #1)
   benchmark_subtraction ======================================
      Sample size:100, total cycles: 32000
      Mean(μ̂): 320.000
      Standard deviation(s): 0.000
      Standard Error(SE): 0.000
      Min: 320 (run #1)
      Max: 320 (run #1)
   void_test ==================================================
      Sample size:100, total cycles: 0
      Mean(μ̂): 0.000
      Standard deviation(s): 0.000
      Standard Error(SE): 0.000
      Min: 0 (run #1)
      Max: 0 (run #1)
   [00:00:00.200,000] <dbg> test_benchmarks: suit_teardown: Tearing down math benchmarks suite
