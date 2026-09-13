.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. _sensor-emulators:

Register map emulators
######################

Register map emulators help smoke-test sensor drivers without hardware. They can uncover mistakes
in register addresses, byte order, scaling, or the handling of negative values.

The approach is declarative: describe the sensor's registers and data formats in C tables, using
its datasheet. The framework handles I2C transfers and fills the data registers with test values.
The real driver reads those registers, and the test checks the values it returns.

A model only needs enough detail to exercise the driver's main paths, such as initialization,
configuration writes, and sample reads. It does not need to reproduce every feature of the sensor.
Start from the datasheet, rather than copying the driver's assumptions: otherwise the model can
repeat the same mistakes.

From datasheet to model
***********************

The P3T1755 temperature sensor provides a small example. Its `datasheet, Rev. 1.3
<https://www.nxp.com/docs/en/data-sheet/P3T1755.pdf>`_ describes the registers in table 13 and the
temperature format in section 7.5.2.

Describe the registers
======================

The temperature register, ``Temp``, has address ``0x00``. It is read-only, holds two bytes, and
has a reset value of zero. The configuration register, ``Conf``, has address ``0x01``. It is
writable, holds one byte, and has a reset value of ``0x28``.

These become two entries in a :c:struct:`emul_sensor_reg` table:

.. code-block:: c

   #define P3T1755_REGS(R) \
           R(TEMP, 0x00, .flags = EMUL_SENSOR_REG_RO) \
           R(CONF, 0x01, .bytes = 1, .reset = 0x28)

   EMUL_SENSOR_REG_TABLE_DEFINE(p3t1755_regs, P3T1755_REGS);

Each row gives a short name, an address, and the register properties. Add the remaining registers
in the same way. The macro creates the table and address constants from this list. Names also
appear in debug logs. Use ``EMUL_SENSOR_REG_ADDR(name)`` in channel tables to reference an address.

``EMUL_SENSOR_REG_RO`` makes writes leave ``Temp`` unchanged. Omitting it makes ``Conf`` writable.
Unspecified reset values are zero. The default register width is set when registering the model
below.

This preserves the access rules without writing an I2C handler. A driver cannot rely on a write
to a read-only register changing its contents. Such writes are ignored, not reported as errors.
Accesses to addresses missing from the table return ``-EIO``.

Describe the temperature
========================

The datasheet uses 12-bit two's complement for temperature. The value occupies bits 15 through 4
of ``Temp``, with the sign in bit 15. Bits 3 through 0 are zero. Each count represents 0.0625
degrees Celsius.

Describe that format in a :c:struct:`emul_sensor_channel` table:

.. code-block:: c

   static const struct emul_sensor_channel p3t1755_channels[] = {
           {
                   .chan = SENSOR_CHAN_AMBIENT_TEMP,
                   .reg = EMUL_SENSOR_REG_ADDR(TEMP),
                   .is_signed = true,
                   .bits = 12,
                   .pos = 4,
                   .lsb = 0.0625,
                   .min = -40.0,
                   .max = 125.0,
           },
   };

``bits`` is the number of data bits. ``pos`` is the position of the lowest data bit. ``lsb`` gives
the value of one count in the channel's units. ``min`` and ``max`` give the range for test values.

For example, a test value of -40 degrees Celsius becomes -640 counts. Its 12-bit encoding is
``0xD80``, so the emulator puts ``0xD800`` in ``Temp``.

Register the model
==================

Connect the two tables and set the bus format:

.. code-block:: c

   EMUL_SENSOR_REGMAP_DEFINE(p3t1755_regs, p3t1755_channels,
                            .reg_bytes = 2, .big_endian = true);

Here, registers hold two bytes unless an entry overrides ``bytes``. The most significant byte
comes first on the bus. The macro creates an emulator for each enabled I2C instance of
``DT_DRV_COMPAT``.

See :zephyr_file:`drivers/sensor/nxp/p3t1755/p3t1755_emul.c` for the complete model, including the
two threshold registers. Its CMake file builds the model when
:kconfig:option:`CONFIG_EMUL_SENSOR_REGMAP` is enabled. This option defaults to enabled when
sensor support, I2C, and :kconfig:option:`CONFIG_EMUL` are enabled.

Run the smoke test
******************

The existing :zephyr_file:`tests/drivers/build_all/sensor/src/generic_test.c` uses the
:c:group:`sensor_emulator_backend` to discover each model's channels and value ranges. It supplies
test values, reads them through the driver using RTIO, and checks the decoded results.

Add the sensor to the test's devicetree if it is not already there. The P3T1755 and HMC5883L nodes
are already present. The test enables emulation, which also enables the register map framework:

.. code-block:: console

   west twister -p native_sim -T tests/drivers/build_all/sensor \
     -s drivers.sensor.generic_test -i

Model limits
************

Test values update the data registers immediately. Power modes, conversion delays, and interrupt
delivery are not simulated. For example, storing a shutdown bit in ``Conf`` does not stop updates
to ``Temp``. A passing smoke test therefore does not validate every device behavior.

Tables can also describe writable bit masks, bits cleared on reads or writes, and sample scales
selected by configuration fields. Use these where the driver needs them. See
:c:struct:`emul_sensor_reg` and :c:struct:`emul_sensor_channel` for the available fields.

API reference
*************

.. doxygengroup:: emul_sensor_regmap
