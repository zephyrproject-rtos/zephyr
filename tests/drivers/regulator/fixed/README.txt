Fixed Regulator
###############

This application tests the fixed regulator driver in multiple
configurations.  Like the GPIO driver test it requires that two GPIO
pins be shorted so that changes made to the state of one can be verified
by inspecting the other.

Unlike the GPIO driver test the behavior of a fixed regulator depends
heavily on the properties specified in its devicetree node.  The
``compatible`` property causes the underlying driver implementation to
select between one that supports asynchronous transitions and an
optimized one synchronous transitions.  The initial state of the
regulator, whether it can be changed, and enforced transition delays are
all controlled by devicetree properties.

Because the regulator can only be configured at system startup it is
necessary to test varying configurations in separate test sessions.  The
``testcase.yaml`` file provides a variety of flags intended to cover the
various behaviors.

Only boards for which an overlay is present can pass this test.  Boards
without an overlay, or for which the required wiring is not provided,
will fail with an error like this:

    Assertion failed at ../src/main.c:182: test_preconditions: (precheck not equal to PC_OK)
    precheck failed: active check failed

No special build options are required to make use of the overlay.
