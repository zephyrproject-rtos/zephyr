.. _i2c_slave_api:

I2C Slave API test
##################

Overview
********

Zephyr supports testing drivers for devices on an I2C bus. This works by
creating a slave device for them to talk to. Instead of talking to the actual
hardware, the I2C traffic from i2c_transfer() is rerouted back through the
I2C-controller driver to an emulator, called a slave device. It is a little
like a loopback mode.


Plumbing
********

Using the example of the STM32 I2C controller driver and the Atmel I2C EEPROM
driver, the flow is something like this:

  some code somewhere: eeprom_read()
    Calls eeprom_at2x.c eeprom_at2x_read()
      Calls read_fn which is eeprom_at24_read(), say
        Calls i2c_write_read()
          Calls i2c_transfer()
            Calls api->read() which is i2c_stm32_transfer()
              Gets an interrupt at some point...
                Calls stm32_i2c_event()
                  Sees it is in slave mode, calls stm32_i2c_slave_event()
                    Calls the slave API write_requested()
                      eeprom_slave_write_requested()
                      (Emulation happens here)

The calls then return back up the stack and the resulting data is provided for
the EEPROM device.

To set this up correctly, the test calls i2c_slave_driver_register() to
register the EEPROM as a slave. The flow is something like this:

  Calls i2c_slave_driver_register() with the EEPROM device
    Calls api->driver_register() which is eeprom_slave_register()
      Calls i2c_slave_register()
        Calls api->slave_register() which is i2c_stm32_slave_register()
          Puts the STM32 I2C bus in slave mode


With all of this set up, EEPROM reads and writes are handled by the EEPROM
simulator.


Limitations
***********

Only a single slave device is supported by each I2C controller.
