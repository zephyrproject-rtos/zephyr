Title: Timer Starvation test

The purpose of the test is to detect whether the timer implementation
correctly handles situations where only one timeout is present, and that
timeout is repeatedly rescheduled before it has a chance to fire.  In
some implementations this may prevent the timer interrupt handler from
ever being invoked, which in turn prevents an announcement of ticks.
Lack of tick announcement propagates into a monotonic increase in the
value returned by sys_clock_elapsed().

This test is not run in automatic test suites because it generally takes
minutes, hours, or days to fail, depending on the hardware clock rate
and the tick rate.  By default the test passes if one hour passes
without detecting a failure.

Failure will occur when some counter wraps around.  This may be a
hardware timer counter, a timer driver internal calculation of
unannounced cycles, or the Zephyr measurement of unannounced ticks.

For example a system that uses a 32768-Hz internal timer counter with
24-bit resolution and determines elapsed time by a 24-bit unsigned
difference between the current and last-recorded counter value will fail
at 512 s when the updated counter value is observed to be less than the
last recorded counter.

Systems that use a 32-bit counter of 80 MHz ticks would fail after
53.687 s.
