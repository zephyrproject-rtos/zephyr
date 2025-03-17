Stepper Constant Velocity Tests
###############################

This test suite focuses on the behaviour of stepper drivers that implement movement at a constant
velocity, i.e. without acceleration or decelleration. All stepper drivers with this movement
profile should pass these tests. Note that because of timing constraints tests that pass on real
hardware might fail during simulation. This is normal.

Hardware Setup
==============

Please refer to the stepper_api test suite for the hardware setup, as the setup for these two test
suites is intended to be identical.
