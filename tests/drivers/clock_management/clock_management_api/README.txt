Clock Management API Test
#########################

This test is designed to verify the functionality of the clock management API.
It defines two dummy devices, which will both be clock consumers. In addition,
it defines several dummy clock nodes to verify API functionality. Boards
should configure these dummy devices with clock states as described within
the tests below.

Boards may also use the dummy clock nodes as needed if they do not have a
hardware clock output they can safely reconfigure as part of this testcase.

The following tests will run, using the output clock with name "default":

* Verify that each consumer can apply the clock state named "default",
  and that the queried rates match the property "default-freq" for each
  device.

* Verify that applying the state named "invalid" propagates an
  error to the user. Board devicetree overlays should configure the
  invalid clock state property such that it will apply invalid clock settings.

* Apply the state named "shared" for the first consumer. The shared clock state
  property for this consumer should be set such that the first consumer
  will be notified about a clock rate change, but the second consumer will
  not be. Verify this is the case.

* Apply the state named "shared" for the second consumer. The shared clock state
  property for this consumer should be set such that both consumers will
  be notified about a clock rate change. Verify this is the case. Additionally,
  check that the consumers are now running at the frequency given by
  the "shared_freq" property.

* Apply the state named "locking" for the first consumer. The locking clock
  state property should be set with the "locking-state" property, so that
  the consumer will now reject changes to its clock. Additionally, check
  that the first consumer is now running at the rate given by "locking-freq"
  property.

* Apply the state named "locking" for the second consumer. The locking clock
  state property should be set such that it would modify the clock rate of
  the first consumer if applied. Verify that the state fails to apply.

* Set clock constraints for the first consumer based on the "freq-constraints-0"
  property present on that node. Verify that the resulting frequency is
  "req-freq-0". Boards should define these constraints such that none of
  the statically defined clock states match this request.

* Set clock constraints for the software consumer that are known to be
  incompatible with those set for the first consumer. Verify these constraints
  are rejected

* Set clock constraints for the second consumer based on the
  "freq-constraints-0" property present on that node. Verify that the resulting
  frequency is "req-freq-0", and that the first consumer was notified of the
  frequency change.

* Set clock constraints for the first consumer based on the "freq-constraints-1"
  property present on that node. Verify that the constraints are rejected.
  Boards should define these constraints such that they are incompatible with
  the constraints set by "freq-constraints-0" for the second consumer.

* Set clock constraints for the second consumer based on the
  "freq-constraints-1" property present on that node. Verify that the resulting
  frequency is "req-freq-1". Boards should define these constraints such that
  one of the statically defined constraints could satisfy this request, and such
  that the framework will now select the static state. No check is made if the
  first consumer is notified of this change.

* Request the best ranked clock for the first consumer based on the
  "freq-constraints-2" property present on that node. Verify that the resulting
  frequency is "req-freq-2". Boards should define clock ranks so that the
  framework will chose a clock that does not produce the most accurate frequency
  possible for this request, to verify the framework handles ranks correctly.
  The constraints of the "freq-constraints-2" node should be set such that
  the second consumer will be unable to use the best ranked clock for its
  request, as switching the first consumer to that clock would violate its
  rank constraints. This can be accomplished either by limiting the frequency
  range or the rank.

* Request the best ranked clock for the second consumer based on the
  "freq-constraints-2" property present on that node. Verify that the resulting
  frequency is "req-freq-2". Boards may define the third element of the
  "freq-constraints-2" to any value that allows the correct frequency to be set.
