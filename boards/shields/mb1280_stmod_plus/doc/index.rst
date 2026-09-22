.. _mb1280_stmod_plus:

MB1280 STMod+ fan-out shield
############################

Overview
********

The MB1280 STMod+ fan-out board converts an STMod+ connector to Grove UART,
Grove I2C, ESP-01, and mikroBUS-compatible connectors.

The shield exposes the MB1280 fan-out connectors through the board's
``stmod_plus_connector`` GPIO nexus.

All GPIO-capable pins on the MB1280 mikroBUS-compatible connector are exposed
through ``mikrobus_header``.

Interface aliases are provided by stackable shield variants:

- ``mb1280_stmod_plus_adc`` maps ``mikrobus_adc`` to ``stmod_adc``.
- ``mb1280_stmod_plus_i2c`` maps ``grove_i2c`` and ``mikrobus_i2c`` to
  ``stmod_i2c``.
- ``mb1280_stmod_plus_pwm`` maps ``mikrobus_pwm`` to ``stmod_pwm``.
- ``mb1280_stmod_plus_serial`` maps ``grove_serial``, ``esp_01_serial``, and
  ``mikrobus_serial`` to ``stmod_serial``.
- ``mb1280_stmod_plus_spi`` maps ``mikrobus_spi`` to ``stmod_spi``.

Requirements
************

This shield can be used with a board that exposes an STMod+ connector with the
``stmod_plus_connector`` node label. The board should also provide matching
``stmod_adc``, ``stmod_i2c``, ``stmod_pwm``, ``stmod_serial``, or
``stmod_spi`` labels for the stackable variants used with this shield.

Programming
***********

Include ``--shield mb1280_stmod_plus`` when invoking ``west build``. Add the
stackable variants that match the STMod+ interfaces provided by the board. For
example:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32n6570_dk/stm32n657xx/sb
   :shield: mb1280_stmod_plus,mb1280_stmod_plus_i2c,mb1280_stmod_plus_serial
   :goals: build
