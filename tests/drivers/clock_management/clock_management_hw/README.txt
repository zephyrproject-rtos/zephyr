Clock Management Hardware Test
##############################

This test is designed to verify the functionality of hardware clock trees
implementing the clock management API. It defines one dummy devices, which
will be a clock consumer.

The test will apply five clock states for the dummy device, and verify the
frequency matches an expected value for each state. The states are as
follows:

* clock-request-0: request name "default", frequency set by "default-freq"
  property of consumer node

* clock-request-1: request name "sleep", frequency set by "sleep-freq"
  property of consumer node

* clock-request-2: request name "test1", frequency set by "test1-freq"
  property of consumer node

* clock-request-3: request name "test2", frequency set by "test2-freq"
  property of consumer node

* clock-request-4: request name "test3", frequency set by "test3-freq"
  property of consumer node

Devices should define these states to exercise as many clock node drivers as
possible. One example might be clocking from a PLL in the default state, a
high speed internal oscillator in the sleep state, and a low speed external
oscillator in the test state.

The test will then request several frequencies from the clock subsystem. Each
of these frequencies should be realized exactly. These requests can be used
to validate that runtime clock frequency setting is functional

The following requests will be made:

* freq-req-1
* freq-req-2
* freq-req-3
