PMIC Regulator
###############

This application tests the i2c pmic regulator driver in multiple
configurations. It requires that an output source from the power management IC
be shorted to an ADC input, so that the test can verify the that the pmic is
responding to commands.

Only boards for which an overlay is present can pass this test.  Boards
without an overlay, or for which the required wiring is not provided,
will fail with an error like this:

    Assertion failed at ../src/main.c:196: test_basic: adc_reading >= 200
	is false

No special build options are required to make use of the overlay.
