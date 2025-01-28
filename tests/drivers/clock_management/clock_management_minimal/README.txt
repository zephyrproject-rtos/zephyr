Clock Management Minimal Test
#############################

This test is designed to verify that the clock management API can function
correctly without runtime callbacks or rate setting enabled. It defines one
dummy clock consumer. In addition, it defines several dummy clock nodes to
verify API functionality. Boards should configure these dummy devices with
clock states as described within the tests below.

Boards may also use the dummy clock nodes as needed if they do not have a
hardware clock output they can safely reconfigure as part of this testcase.

The following tests will run, using the output clock with name "default":

* Verify that the consumer can apply the clock state named "default" for
  both the "slow" and "fast" clock output, and that the queried rates of
  the "slow" and "fast" clocks match the properties "slow-default-freq"
  and "fast-default-freq", respectively.

* Verify that the consumer can apply the clock state named "sleep" for
  both the "slow" and "fast" clock output, and that the queried rates of
  the "slow" and "fast" clocks match the properties "slow-sleep-freq"
  and "fast-sleep-freq", respectively.

* Verify that requesting the frequency given by "slow-request-freq" from
  the "slow" clock output reconfigures that clock output to *exactly* the
  frequency given by the "slow-request-freq" property.

* Verify that requesting the frequency given by "fast-request-freq" from
  the "fast" clock output reconfigures that clock output to *exactly* the
  frequency given by the "fast-request-freq" property.
