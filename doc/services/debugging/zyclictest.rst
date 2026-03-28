.. _zyclictest:

Zyclictest
##########

The zyclictest module enables estimation of worst case latencies of realtime
threads. It can measure the time from the hardware interrupt to the service
routine and further to the Zephyr thread.

This module and its name is inspired from the cyclictest program on Linux.
That's why it's a Zephyr cyclictest.

The idea is to use the timer interrupt as interrupt source as we know exactly
the point of time when it occurred. In the interrupt service routine we take
the time and calculate the difference to the programmed time of the timer
interrupt.

Further a thread is also synchronizing itself on the timer and also measures
the difference to the programmed timer.

Both times are put into an array which serves as data source of the histogram
printed out at the end of the measurement.

This measurement is done in a cyclic manner with a certain interval time.

If someone wants to know the worst case latency of a task with priority <p>
then the application needs to be running. While it is running we can start
zyclictest with a priority at least one digit less than the application thread
probed. For example if our application with the thread of interrest has the
priority -10 than we need to start zyclictest with a priority of -11 or less.

Another important argument is the interval time. It's not meant to have an
interval less than the measured worst case latency. Therefore it's recommended
to set up an interval with at least double the time of the expected or measured
worst case latency.

If there are overflows indicated at the end of the measurement than this means
that there is no deterministic worst case latency inside the range of the
histogram.

For a meaningful output it's recommended to set up
:kconfig:option:`CONFIG_SYS_CLOCK_TICKS_PER_SEC` to at least 1000000 as this
means a minimum resolution of 1 microsecond. A tickless kernel is also needed
(:kconfig:option:`CONFIG_TICKLESS_KERNEL`:).

Mode of operation
*****************

Zyclictest can be started free running without a fixed number of cycles. This
is the default. When stopped the result so far is printed out.

In the loop mode (option start with -l <loops>) zyclictest is running for a
predefined number of cycles. When the number of cycles is reached there is a
message on the shell indicating the end of the test. But the test can be
canceled prematurely with zyclictest stop -c.

Configuration
*************

Configure this module using the following options.

* :kconfig:option:`CONFIG_ZYCLICTEST_SHELL`: enable the shell command.


Usage
*****

zyclictest start [options]
  -i <interval>  Interval in microseconds
  -l <loops>     Use loop mode with predefined number of cycles
  -p <prio>      Setup priority of Thread

zyclictest stop [options]
  -c             Cancel loop mode prematurely
  -q             Quiet mode, print summary, but no histogram data

Example
*******

In this example we want to know the worst case latency of a thread which is
woken up by an interrupt and running as cooperative task with priority -10. We
expect a latency of less then 200 us. We are using the free running mode.

1. Start the zyclictest thread with an interval of 400 us (double of expected
worst case latency) and a priority of -11 (one digit higher than the
application thread):

   .. code-block:: console

      zyclictest start -i 400 -p -11

2. Do whatever should be tested....

3. Stop the measurement:

   .. code-block:: console

      zyclictest stop

   Output from zyclictest:

   ::

      Count: 547329
                         IRQ  Thread
      Max-Latency:        21      27
      Errors:              0       0
      Overflow:            0       0
      Histogram:
      [...]
       23                  0  547306
       24                  0       2
       25                  0       5
       26                  0       5
       27                  0       2
       28                  0       0
      [...]

4. Interpret the result:

   Where are no errors and no overflows which means the measurement can be used.
   There were 547329 cycles in the test.
   Worst case interrupt latency is 21 us and for threads it's 27 us.
