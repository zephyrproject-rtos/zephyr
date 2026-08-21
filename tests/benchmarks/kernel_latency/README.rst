Kernel Latency Benchmark
########################

This benchmark measures the cost of the kernel operations an application
uses constantly: creating and switching threads, taking and giving
synchronisation objects, moving items through queues, and allocating from
the heap.

It covers the same ground as ``tests/benchmarks/latency_measure``, which
remains in place, but is built on the :ref:`ztest benchmarking framework
<ztest_benchmarking>` rather than on its own harness. What that buys is
percentile reporting, control-measurement correction and twister-recordable
output for free, and one benchmark per operation rather than one number per
printed line.

Structure
*********

Each kernel object gets a suite of its own in a file of its own, because
``ZTEST_BENCHMARK_SUITE()`` defines the suite object static and a benchmark
has to live in the same translation unit as its suite.

Operations that begin and end in the calling thread are plain
``ZTEST_BENCHMARK()`` bodies, which the framework times itself. Where an
operation has a natural pair -- lock and unlock, put and get, malloc and
free -- the two are separate benchmarks rather than one round trip: the
setup and teardown hooks are not timed, so whichever half is not being
measured can be done there. That is how ``mutex.lock`` and ``mutex.unlock``
come out as separate numbers.

Operations that end in a *different* thread are
``ZTEST_BENCHMARK_MANUAL()`` bodies. The waking thread takes the first
timestamp and the woken thread takes the second, which framework-timed
benchmarks cannot express. These are the ``*_wake_switch`` benchmarks and
the two context switch measurements.

What is measured
****************

===================================  ==========================================
Suite / benchmark                    Operation
===================================  ==========================================
``thread.create``                    ``k_thread_create()`` without starting
``thread.start``                     ``k_thread_start()`` on a lower priority
                                     thread, so no switch occurs
``thread.suspend`` / ``resume``      suspend and resume a started thread
``thread.abort``                     abort a started thread
``thread.yield_preemptive``          context switch between preemptible
                                     threads of equal priority
``thread.yield_cooperative``         the same between cooperative threads
``semaphore.give`` / ``take``        uncontended, no waiter
``semaphore.give_wake_switch``       give to a blocked higher priority thread,
                                     until that thread runs
``mutex.lock`` / ``unlock``          uncontended
``fifo.put`` / ``get``               uncontended
``fifo.alloc_put``                   ``k_fifo_alloc_put()``, which takes memory
                                     from the thread resource pool
``fifo.put_wake_switch``             put to a blocked higher priority thread
``lifo.put`` / ``get``               uncontended, for comparison with the FIFO
``stack.push`` / ``pop``             uncontended
``heap.malloc`` / ``free``           64 byte block from the kernel heap
``events.post`` / ``wait_satisfied`` post, and a wait that is already satisfied
``events.post_wake_switch``          post to a blocked higher priority thread
``condvar.signal_wake_switch``       signal a waiting thread; necessarily
                                     includes the mutex handoff
===================================  ==========================================

Running
*******

.. code-block:: console

   west build -p -b <board> tests/benchmarks/kernel_latency -t run -- \
       -DCONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE=y

Without the override the application selects CSV output, which is what
twister records:

.. code-block:: console

   scripts/twister -p qemu_x86 -T tests/benchmarks/kernel_latency

Warmup and the cold cost
************************

Each benchmark is setup, then
:kconfig:option:`CONFIG_ZTEST_BENCHMARK_WARMUP` iterations that execute but
are not recorded, then the measured samples. The framework runs the warmup
and reports the very first execution separately as the **cold cost**, so
the two are never mixed.

That separation is not academic. On an FRDM-MCXN947 the first execution
costs between 1.1 and 1.8 times the steady state: ``thread.suspend`` is 343
cycles cold against 196 hot, ``mutex.lock`` 231 against 137. Folded into
the distribution a single sample like that moves the maximum and the
standard deviation while describing a state the benchmark never returns to;
reported on its own it answers a question an application actually has,
which is what a path costs the first time it is reached.

Reading the results
*******************

Percentiles are enabled, so each benchmark reports ``min``, ``p50``,
``p90``, ``p99``, ``p99.9``, ``p99.99`` and ``max`` as well as the mean and
standard deviation. For kernel operations the median is usually the number
of interest -- these are short, uncontended paths -- but the tail is what
shows when an operation is occasionally interrupted, and the two differ by
more than the mean suggests.

At the default thousand samples p99.9 is the last percentile with enough
samples behind it. Raise :kconfig:option:`CONFIG_KERNEL_BENCH_NUM_SAMPLES`
and :kconfig:option:`CONFIG_ZTEST_BENCHMARK_MAX_SAMPLES` together to go
further out.

The cold cost is reported for every benchmark, and in CSV output it is a
row of its own.

Notes
*****

* Memory mapped thread stacks are disabled. The thread benchmarks create and
  destroy a thread on the same stack once per sample, and each cycle consumes
  virtual address space that is not handed back, so a few hundred iterations
  exhaust the mapping pool and ``k_thread_create()`` asserts. Disabling it
  also keeps thread creation comparable with platforms that have no MMU.
* Absolute cycle counts under emulation say little. QEMU advances its cycle
  counter with host time rather than with emulated instructions, so numbers
  taken there are useful for comparing operations against each other and
  useless as absolute figures. Take them on hardware.
* Userspace variants of these operations, which ``latency_measure``
  reports, are not covered yet.
