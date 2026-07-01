Title: Test to verify the SW Vector Relay feature (ARM Only)

Description:

This test verifies that the software vector table relay feature
(CONFIG_SW_VECTOR_RELAY=y) works as expected. It runs only on ARM
Cortex-M targets.

The arm_sw_vector_relay suite verifies that the relay vector table entries
(other than the first two, reserved for the initial MSP and the reset
handler) all point to the relay handling function, and that the vector
table pointer is set up correctly: on targets with VTOR the forwarding and
real vector tables respect the VTOR.TBLOFF alignment requirement and VTOR
points to the real table, while on targets without VTOR the vector table
pointer points to the start of the real vector table.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on an ARM Cortex-M QEMU platform:

    twister -p mps2/an385 -T tests/arch/arm/arm_sw_vector_relay

Or build and run a single platform directly with west:

    west build -b mps2/an385 tests/arch/arm/arm_sw_vector_relay
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE arm_sw_vector_relay
===================================================================
START - test_arm_sw_vector_relay
 PASS - test_arm_sw_vector_relay
===================================================================
TESTSUITE arm_sw_vector_relay succeeded
