Clock Management API Test
#########################

This test is designed to verify the functionality of the clock management API.
It defines two dummy devices, which will both be clock consumers. In addition,
it defines several dummy clock nodes to verify API functionality. Boards
should configure these dummy devices with clock states as described within
the tests below.

Boards may also use the dummy clock nodes as needed if they do not have a
hardware clock output they can safely reconfigure as part of this testcase.

The following tests will run:

* Verify that each consumer can apply the clock state CLOCK_MGMT_STATE_DEFAULT,
  and that the queried rates match the property "default-freq" for each
  device.

* Verify that applying the state CLOCK_MGMT_STATE_SLEEP propagates an
  error to the user. Board devicetree overlays should configure the
  clock-state-1 property such that it will apply invalid clock settings.

* Apply CLOCK_MGMT_STATE_SHARED for the first consumer. The clock-state-2
  property for this consumer should be set such that the first consumer
  will be notified about a clock rate change, but the second consumer will
  not be. Verify this is the case.

* Apply CLOCK_MGMT_STATE_SHARED for the second consumer. The clock-state-2
  property for this consumer should be set such that both consumers will
  be notified about a clock rate change. Verify this is the case. Additionally,
  check that the consumers are now running at the frequency given by
  the "shared_freq" property.

* Apply CLOCK_MGMT_SETRATE state for the first consumer, and verify that
  the resulting frequency matches the "setrate-freq" property. Board devicetrees
  should define clock-state-3 using an output clock node, to test the
  clock_set_rate API.

* Verify that the first consumer now holds a lock on the clock nodes used to
  produce its frequency by requesting a rate for the second consumer that would
  be best satisfied by the nodes the first consumer already has a lock on. The
  test will check that the resulting frequency matches the "setrate-freq"
  property for the second consumer. Board devicetrees should define
  clock-state-3 using an output clock node, with a frequency that would
  best be satisfied by clock nodes the first consumer will be using

* Verify both consumers are able to exercise their locks by attempting
  to reconfigure clocks to new input rates. Board devicetrees should define
  clock-state-4 using output clock nodes. Both consumers will have their
  frequencies checked against the "setrate1-freq" property to verify they match.
