Title: ARC VPX Lock

Description:

This test verifies that the ARC VPX lock/unlock mechanism used to bookend
code that uses the ARC VPX vector registers works correctly. As this VPX
lock/unlock mechanism does not technically require those registers to be
used to control access to them (they bookend the relevant code sections),
the test does not actually access those VPX registers.

However, it does check that the system behaves as expected when the ARC VPX
lock/unlock mechanism is used.
