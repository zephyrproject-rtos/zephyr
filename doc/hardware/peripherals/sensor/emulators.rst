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

The HMC5883L magnetometer provides a compact example. Its datasheet, Honeywell form 900405 Rev. E,
describes the registers in tables 2 to 20.

Describe the registers
======================

The HMC5883L has 13 registers of one byte each. Configuration register B, ``CRB``, has address
``0x01``. It is writable and has a reset value of ``0x20``. The X output is split between
``DXRA``, which holds the most significant byte, at ``0x03`` and ``DXRB`` at ``0x04``. Both are
read-only, like the status register, ``SR``, at ``0x09``. The identification registers ``IRA``
to ``IRC`` read back the ASCII string ``H43``.

These become entries in a :c:struct:`emul_sensor_reg` table:

.. code-block:: c

   #define HMC5883L_REGS(R) \
           R(CRB, 0x01, .reset = 0x20) \
           R(DXRA, 0x03, .flags = EMUL_SENSOR_REG_RO) \
           R(DXRB, 0x04, .flags = EMUL_SENSOR_REG_RO) \
           R(SR, 0x09, .flags = EMUL_SENSOR_REG_RO) \
           R(IRA, 0x0A, .flags = EMUL_SENSOR_REG_RO, .reset = 0x48)

   EMUL_SENSOR_REG_TABLE_DEFINE(hmc5883l_regs, HMC5883L_REGS);

Each row gives a short name, an address, and the register properties. Add the remaining registers
in the same way. The macro creates the table and address constants from this list. Names also
appear in debug logs. Use ``EMUL_SENSOR_REG_ADDR(name)`` in channel tables to reference an address.

``EMUL_SENSOR_REG_RO`` makes writes leave ``DXRA`` unchanged. Omitting it makes ``CRB`` writable.
Unspecified reset values are zero. Reset values such as the identification bytes let the driver
find the device it expects. The default register width is set when registering the model below.

This preserves the access rules without writing an I2C handler. A driver cannot rely on a write
to a read-only register changing its contents. Such writes are ignored, not reported as errors.
Accesses to addresses missing from the table return ``-EIO``.

Describe the magnetic field
===========================

Each axis is a 16-bit two's complement value that holds the output of a 12-bit ADC, from
``0xF800`` to ``0x07FF``. Its scale depends on the gain field, bits 7 through 5 of ``CRB``: from
1370 counts per gauss for setting 0 to 230 counts per gauss for setting 7. A new sample sets the
ready bit, bit 0 of ``SR``.

Describe that format in a :c:struct:`emul_sensor_channel` table, shown here for the X axis:

.. code-block:: c

   static const struct emul_sensor_channel hmc5883l_channels[] = {
           {
                   .chan = SENSOR_CHAN_MAGN_X,
                   .reg = EMUL_SENSOR_REG_ADDR(DXRA),
                   .is_signed = true,
                   .bits = 16,
                   .min = -1.3,
                   .max = 1.3,
                   .select = {EMUL_SENSOR_REG_ADDR(CRB), GENMASK(7, 5)},
                   .variants = {
                           {.lsb = 1.0 / 1370},
                           {.lsb = 1.0 / 1090},
                           /* ... one entry per gain setting, down to 1.0 / 230 */
                   },
                   .ready = {EMUL_SENSOR_REG_ADDR(SR), BIT(0)},
           },
   };

``reg`` is the first register of the data word. ``bits`` is the number of data bits. ``pos``, the
position of the lowest data bit, is zero here; a value aligned to the top of its register sets it
to the number of unused low bits. ``min`` and ``max`` give the range for test values: 1.3 gauss
fits the ADC at every gain.

``select`` names the configuration field whose value indexes ``variants``. Each variant here only
replaces ``lsb``, the value of one count in the channel's units. ``ready`` gives the status bits
to set when a sample is stored.

For example, the reset value of ``CRB`` selects gain setting 1. A test value of 0.5 gauss then
becomes 545 counts, so the emulator puts ``0x02`` in ``DXRA`` and ``0x21`` in ``DXRB``, and sets
bit 0 of ``SR``. The Y and Z axes are described in the same way. Their registers come in X, Z, Y
order, a detail that a driver can get wrong.

Register the model
==================

Connect the two tables and set the bus format:

.. code-block:: c

   EMUL_SENSOR_REGMAP_DEFINE(hmc5883l_regs, hmc5883l_channels, .big_endian = true);

Registers hold one byte unless ``.reg_bytes`` or an entry's ``bytes`` says otherwise, so a 16-bit
sample spans two consecutive registers. ``.big_endian`` puts its most significant byte first. The
macro creates an emulator for each enabled I2C instance of ``DT_DRV_COMPAT``.

See :zephyr_file:`drivers/sensor/honeywell/hmc5883l/hmc5883l_emul.c` for the complete model. Its
CMake file builds the model when :kconfig:option:`CONFIG_EMUL_SENSOR_REGMAP` is enabled. This
option defaults to enabled when sensor support, I2C, and :kconfig:option:`CONFIG_EMUL` are
enabled.

Run the smoke test
******************

The existing :zephyr_file:`tests/drivers/build_all/sensor/src/generic_test.c` uses the
:c:group:`sensor_emulator_backend` to discover each model's channels and value ranges. It supplies
test values, reads them through the driver using RTIO, and checks the decoded results.

Add the sensor to the test's devicetree if it is not already there. The HMC5883L node is already
present. The test enables emulation, which also enables the register map framework:

.. code-block:: console

   west twister -p native_sim -T tests/drivers/build_all/sensor \
     -s drivers.sensor.generic_test -i

Model limits
************

Test values update the data registers immediately. Power modes, conversion delays, and interrupt
delivery are not simulated. For example, selecting the idle mode in ``MR`` does not stop updates
to the data registers. A passing smoke test therefore does not validate every device behavior.

Tables can also describe writable bit masks, bits cleared on reads or writes, and offsets. Use
these where the driver needs them. See
:c:struct:`emul_sensor_reg` and :c:struct:`emul_sensor_channel` for the available fields.

API reference
*************

.. doxygengroup:: emul_sensor_regmap
