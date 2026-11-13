Overview
********

The test verifies the features offered by the ``hid_report.c`` module.
It includes several real-world report descriptors and checks that the parser
correctly handles them, as well as a few input reports.

The features verified by the test are detached from the hardware.
It can be ran either on the ``native_sim`` target or any real hardware.
