Overview
********

The test creates a mock HID device and verifies that the communication between
it and the USB Host stack work as expected, by receiving input reports, attempting
to change the protocol and idle rates and sending an output report to the device.

The test can be ran either on the native_sim target (in which case all communication
passes through the virtual USB stack) or the ``ek_ra8m2`` development kit.

If executed on the development kit one should loop the USB HS interface with the
USB FS interface: the test will expose a HID device on the FS interface and the host
stack will run on the HS interface.
