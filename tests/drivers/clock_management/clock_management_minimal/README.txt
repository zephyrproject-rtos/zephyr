Clock Management Minimal Test
#############################

This test is designed to verify that the clock management API can function
correctly without runtime callbacks or rate setting enabled. It defines one
dummy clock consumer. In addition, it defines several dummy clock nodes to
verify API functionality. Boards should configure these dummy devices with
clock requests as described within the tests below.

Boards may also use the dummy clock nodes as needed if they do not have a
hardware clock output they can safely reconfigure as part of this testcase.

The following tests will run, using the output clock with name "default":

* Verify that the consumer can apply the clock request named "default"
  and that the queried rates of the "slow" and "fast" clocks match the
  properties "slow-default-freq" and "fast-default-freq", respectively.

* Verify that the consumer can apply the clock request named "sleep"
  and that the queried rates of the "slow" and "fast" clocks match the properties
  "slow-sleep-freq" and "fast-sleep-freq", respectively.
