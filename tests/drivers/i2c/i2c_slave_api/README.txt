I2C Slave API test
##################

.. note:
   See :ref:`coding_guideline_inclusive_language` for information about
   plans to change the terminology used in this API.

This test verifies I2C slave driver implementations using two I2C
controllers on a common bus.  The test is supported by a test-specific
driver that simulates an EEPROM with an I2C bus slave interface.  Data
is pre-loaded into the simulated devices outside the I2C API, and the
Zephyr application issues commands to one controller that are responded
to by the simulated EEPROM connected through the other controller.

This test was originally designed for I2C controllers that support both
master and slave behavior simultaneously.  This is not true of all
I2C controllers, so this behavior is now opt-in using
CONFIG_APP_DUAL_ROLE_I2C.  However, the devicetree still must provide a
second EEPROM just to identify the bus.

In slightly more detail the test has these phases:

* Use API specific to the simulated EEPROM to pre-populate the simulated
  devices with device-specific content.
* Register a simulated EEPROM as a I2C slave device on a bus.  If
  CONFIG_APP_DUAL_ROLE_I2C is selected, register both.

* Issue commands on one bus controller (operating as the bus master) and
  verify that the data supplied by the other controller (slave) match
  the expected values given the content known to be present on the
  simulated device.  If CONFIG_APP_DUAL_ROLE_I2C is selected, do this
  with the roles reversed.

Transfer of commands from one bus controller to the other is
accomplished by hardware through having the SCL (and SDA) signals
shorted to join the two buses.

Presence of this required hardware configuration is identified by the
`i2c_bus_short` fixture.  If the buses are not connected as required,
or the controller driver has bugs, the test will fail one or more I2C
transactions.
