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

* Verify that each consumer can request clock state named "default",
  and that the queried rates match the property "default-freq" for each
  device.

* Verify that requesting the state named "invalid" propagates an
  error to the user. Board devicetree overlays should configure the
  invalid clock state property such that it will apply invalid clock settings.

* Request the state "ranked" for the first consumer, and verify that the frequency
  matches the "ranked-freq" parameter. This state should allow multiple clock
  states for the consumer, and the lowest ranked state should be chosen. This
  state should be defined such that only the first consumer is notified of a
  clock state change.

* Request the state "shared" for the second consumer. This state should be defined
  such that the first consumer's clock output will change state. Verify that the
  first consumer now is clocked at "shared-freq" frequency, and that it
  was notified of the frequency change. Verify that the second consumer is clocked
  at "shared-freq".

* Switch the second consumer to the "ranked" request. Verify it is now using
  the "ranked-freq", and that the first consumer has returned to the
  "ranked-freq" frequency. This state should be defined so that it only
  influences the second consumer's clock.

* Set the first consumer to the default clock state, and lock the clock output
  in use by the first consumer. Try to request the state named "locked" for the
  second consumer. The locked clock state request should be set such that it
  would modify the clock rate of the first consumer if applied.
  Verify that the state fails to apply.

* Request the frequency given by "freq-req-1" for the first consumer. Verify
  the clock reconfigures to "req-freq-1". Lock the clock so that the second
  consumer won't be able to reconfigure it.

* Request the frequency given by "freq-req-1" for the second consumer. Verify
  the clock reconfigures to "req-freq-1".
