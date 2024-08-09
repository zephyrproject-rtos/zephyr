Clock Management Hardware Test
##############################

This test is designed to verify the functionality of hardware clock trees
implementing the clock management API. It defines one dummy devices, which
will be a clock consumer.

The test will apply five clock states for the dummy device, and verify the
frequency matches an expected value for each state. The states are as
follows:

* clock-state-0: CLOCK_MGMT_STATE_DEFAULT, frequency set by "default-freq"
  property of consumer node

* clock-state-1: CLOCK_MGMT_STATE_SLEEP, frequency set by "sleep-freq"
  property of consumer node

* clock-state-2: CLOCK_MGMT_STATE_TEST1, frequency set by "test1-freq"
  property of consumer node

* clock-state-3: CLOCK_MGMT_STATE_TEST2, frequency set by "test2-freq"
  property of consumer node

* clock-state-4: CLOCK_MGMT_STATE_TEST3, frequency set by "test3-freq"
  property of consumer node

Devices should define these states to exercise as many clock node drivers as
possible. One example might be clocking from a PLL in the default state, a
high speed internal oscillator in the sleep state, and a low speed external
oscillator in the test state. Clock output nodes should also be used for some
of these states, to verify that the hardware implements clock_set_rate and
clock_get_rate as expected.
