.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. _sensor-emulators:

Register map emulators
######################

Register map emulators let tests inject sensor values and exercise a real driver over an emulated
I2C bus. Enable :kconfig:option:`CONFIG_EMUL_SENSOR_REGMAP` and place the sensor node under a
:dtcompatible:`zephyr,i2c-emul-controller` node. Use the :c:group:`sensor_emulator_backend` to set
channel values before calling the driver's sample fetch or read API.

Writing a model
***************

Describe the datasheet's register addresses, reset values, permissions, and channel encodings in
:c:struct:`emul_sensor_reg` and :c:struct:`emul_sensor_channel` tables. Channel scales use the units
of the sensor API, including degrees Celsius and gauss. Consecutive registers holding a channel
must have the same width. A device's byte order applies to both transfers and channel values.

:c:macro:`EMUL_SENSOR_REGMAP_DEFINE` creates an emulator for each enabled I2C instance of
``DT_DRV_COMPAT``. Add the model source to the driver's CMake file under
``CONFIG_EMUL_SENSOR_REGMAP``.

Unlisted registers return ``-EIO``; writes to read-only registers are ignored. Register tables can
limit writable bits, clear command bits on writes, and clear status bits after reads. Channel
tables can select encodings from configuration bits and set status bits when samples are injected.
See the API reference for field defaults.

Scope and testing
*****************

Injection updates output registers immediately, rounding to the nearest count with halfway values
rounded away from zero. These models exercise register access and sample conversion. They do not
simulate measurement timing, power modes, interrupts, analog behavior, or command side effects
beyond the declared bit changes. Configuration storage alone does not simulate a feature.

The existing generic sensor test discovers models implementing the backend API and checks
injected values through RTIO read and decode. Enable the emulator in that test with:

.. code-block:: console

   west twister -p native_sim -T tests/drivers/build_all/sensor \
     -s drivers.sensor.generic_test -i

API reference
*************

.. doxygengroup:: emul_sensor_regmap
