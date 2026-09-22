.. Copyright (c) 2026 Analog Devices Inc.
.. SPDX-License-Identifier: Apache-2.0

LTC4286 Sensor Test Suite
#########################

This test suite provides comprehensive validation of sensor attributes and alert
mask configuration for the LTC4286 Hot Swap Controller and Power Monitor.

Overview
********

The LTC4286 is a hot swap controller with precision monitoring capabilities.
This test suite validates:

- Threshold configuration attributes (voltage, current, power, temperature)
- Device configuration attributes (operation, clear faults, write protect)
- ADC configuration attributes
- Manufacturer configuration attributes
- Fault response attributes
- Reboot control attributes
- Read-only manufacturer status attributes
- Write protection logic
- Alert mask enable/disable API
- Invalid attribute and boundary value handling

Test Structure
**************

Test Files
==========

1. ``src/ltc4286_attr_test.c`` - Attribute validation tests (18 test cases)
2. ``src/ltc4286_alert_test.c`` - Alert mask API tests (21 test cases)
3. ``src/ltc4286_emul.c`` - I2C emulator for the LTC4286 device
4. ``src/ltc4286_emul.h`` - Emulator header file
5. ``boards/native_sim.overlay`` - Device tree overlay for test configuration
6. ``testcase.yaml`` - Twister test case definition

Test Suites
===========

Suite 1: ltc4286_attrs (ltc4286_attr_test.c)
--------------------------------------------

Threshold Attributes (test_threshold_\*_attrs)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tests for setting and getting warning thresholds:

Voltage Thresholds:

- ``SENSOR_ATTR_LTC4286_THRESH_VOUT_OV_WARN`` - Output voltage overvoltage warning
- ``SENSOR_ATTR_LTC4286_THRESH_VOUT_UV_WARN`` - Output voltage undervoltage warning
- ``SENSOR_ATTR_LTC4286_THRESH_VIN_OV_WARN`` - Input voltage overvoltage warning
- ``SENSOR_ATTR_LTC4286_THRESH_VIN_UV_WARN`` - Input voltage undervoltage warning

Current Thresholds:

- ``SENSOR_ATTR_LTC4286_THRESH_IOUT_OC_WARN`` - Output current overcurrent warning

Power Thresholds:

- ``SENSOR_ATTR_LTC4286_THRESH_PIN_OP_WARN`` - Output power warning

Temperature Thresholds:

- ``SENSOR_ATTR_LTC4286_THRESH_TEMP_OT_WARN`` - Temperature overtemperature warning
- ``SENSOR_ATTR_LTC4286_THRESH_TEMP_UT_WARN`` - Temperature undertemperature warning

Device Configuration Attributes (test_config_attrs)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tests for core PMBus configuration:

- ``SENSOR_ATTR_LTC4286_CONFIG_OPERATION`` - Device operation mode
- ``SENSOR_ATTR_LTC4286_CONFIG_CLEAR_FAULTS`` - Clear faults (send-only command)
- ``SENSOR_ATTR_LTC4286_CONFIG_WRITE_PROTECT`` - Write protection control

ADC Configuration Attributes (test_adc_config_attrs)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tests for ADC configuration:

- ``SENSOR_ATTR_LTC4286_ADC_VDS_SELECT`` - VDS measurement selection
- ``SENSOR_ATTR_LTC4286_ADC_VIN_VOUT_SELECT`` - VIN/VOUT measurement selection
- ``SENSOR_ATTR_LTC4286_ADC_DISP_AVG`` - Display averaged values
- ``SENSOR_ATTR_LTC4286_ADC_AVG_SELECT`` - Averaging mode selection (0-15)

Manufacturer Configuration Attributes (test_mfr_config_attrs)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tests for manufacturer-specific configuration:

- ``SENSOR_ATTR_LTC4286_MFR_CONFIG_ILIM`` - Current limit configuration (0-15)
- ``SENSOR_ATTR_LTC4286_MFR_CONFIG_VRANGE_SEL`` - Voltage range selection
- ``SENSOR_ATTR_LTC4286_MFR_CONFIG_PMBUS_1MBIT`` - PMBus 1Mbit speed mode
- ``SENSOR_ATTR_LTC4286_MFR_CONFIG_EXT_TEMP`` - External temperature sensor
- ``SENSOR_ATTR_LTC4286_MFR_CONFIG_DEBOUNCE_TIMER`` - Input debounce timer

Fault Response Attributes (test_fault_response_\*_attrs)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tests for fault response configuration across different fault types:

Overcurrent (IOUT_OC):

- ``SENSOR_ATTR_LTC4286_FAULT_RESPONSE`` - Fault response mode (0-3)
- ``SENSOR_ATTR_LTC4286_FAULT_RETRY`` - Fault retry count (0-7)

Input Overvoltage (VIN_OV):

- ``SENSOR_ATTR_LTC4286_FAULT_OV_RESPONSE``
- ``SENSOR_ATTR_LTC4286_FAULT_OV_RETRY``

Input Undervoltage (VIN_UV):

- ``SENSOR_ATTR_LTC4286_FAULT_UV_RESPONSE``
- ``SENSOR_ATTR_LTC4286_FAULT_UV_RETRY``

FET Fault:

- ``SENSOR_ATTR_LTC4286_FAULT_RESPONSE``
- ``SENSOR_ATTR_LTC4286_FAULT_RETRY``

Output Power (PIN_OP):

- ``SENSOR_ATTR_LTC4286_FAULT_RESPONSE``
- ``SENSOR_ATTR_LTC4286_FAULT_RETRY``
- ``SENSOR_ATTR_LTC4286_FAULT_OP_TIMER`` - Operation timer

Reboot Control Attributes (test_reboot_control_attrs)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tests for reboot control:

- ``SENSOR_ATTR_LTC4286_REBOOT_CONTROL_DELAY`` - Reboot delay time (0-7)

Read-Only Attributes (test_read_only_mfr_attrs)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Validates read-only manufacturer attributes:

- ``SENSOR_CHAN_LTC4286_MFR_COMMON`` - Common manufacturer settings
- ``SENSOR_CHAN_LTC4286_MFR_PADS_LIVE_STATUS`` - Pads live status
- ``SENSOR_CHAN_LTC4286_MFR_SHUTDOWN_CAUSE`` - Shutdown cause

Write Protection Tests
~~~~~~~~~~~~~~~~~~~~~~

``test_write_protect_attrs`` and ``test_write_protect_logic`` validate:

- WRITE_PROTECT register is correctly written at the emulator level
- Writes to protected attributes return ``-EACCES`` when protection is active
- Disabling write protection is always allowed
- After clearing protection, writes succeed again

Invalid Attribute Testing (test_invalid_attrs)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Validates error handling:

- Attempts to set read-only channels (should return ``-ENOTSUP``)
- Invalid channel numbers (should return ``-EINVAL``)
- Invalid attribute operations

Boundary Value Testing (test_threshold_boundary_values)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tests edge cases:

- Minimum voltage threshold (0V)
- Maximum temperature threshold (125C)
- Negative temperature threshold (-40C)

Suite 2: ltc4286_alert (ltc4286_alert_test.c)
----------------------------------------------

Tests for the ``ltc4286_enable_alert_mask()`` / ``ltc4286_disable_alert_mask()``
API.

Semantics: bit SET (1) = alert masked (disabled), bit CLEAR (0) = alert
unmasked (enabled).

Alert Mask Registers Tested
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1

   * - Register
     - Bits tested
   * - ``LTC4286_ALERT_STATUS_VOUT_MASK``
     - OV warning, UV warning
   * - ``LTC4286_ALERT_STATUS_IOUT_MASK``
     - OC fault, OC warning
   * - ``LTC4286_ALERT_STATUS_INPUT_MASK``
     - VIN OV fault/warning, VIN UV fault/warning, PIN OP warning
   * - ``LTC4286_ALERT_STATUS_TEMP_MASK``
     - OT fault, OT warning, UT warning
   * - ``LTC4286_ALERT_STATUS_COMMS_MASK``
     - Bad cmd, Bad data, PEC failed, Misc fault
   * - ``LTC4286_ALERT_STATUS_SPEC_MASK``
     - EN changed, TSD fault, FET bad fault
   * - ``LTC4286_ALERT_STATUS_WORD_MASK``
     - Busy
   * - ``LTC4286_ALERT_STATUS_SYS1_MASK``
     - ADC conv, Average done, Power loss, Reset done
   * - ``LTC4286_ALERT_STATUS_SYS2_MASK``
     - FET short, IOUT UC, PIN UP, Power failed, VDS OV/UV

Alert Test Categories
~~~~~~~~~~~~~~~~~~~~~

- **Disable tests** (``test_disable_*_alert_sets_bit``) - Verify masking sets
  the bit
- **Enable tests** (``test_enable_*_alert_clears_bit``) - Verify unmasking
  clears the bit
- **Toggle test** (``test_alert_toggle``) - Disable/enable/disable cycle
- **Independence test** (``test_alert_bit_independence``) - Operating on one bit
  does not affect others
- **Invalid input** (``test_invalid_alert_mask_returns_einval``) - Unsupported
  register returns ``-EINVAL``

Running the Tests
*****************

Build and Run
=============

.. code-block:: bash

   cd $ZEPHYR_BASE
   west build -p -b native_sim tests/drivers/sensor/ltc4286
   west build -t run

Using Twister
=============

.. code-block:: bash

   west twister -T tests/drivers/sensor/ltc4286

Expected Output
===============

The test suite should pass all 39 test cases:

ltc4286_attrs (18 tests):

- test_threshold_voltage_attrs
- test_threshold_current_attrs
- test_threshold_power_attrs
- test_threshold_temperature_attrs
- test_adc_config_attrs
- test_config_attrs
- test_mfr_config_attrs
- test_fault_response_iout_oc_attrs
- test_fault_response_vin_ov_attrs
- test_fault_response_vin_uv_attrs
- test_fault_response_fet_attrs
- test_fault_response_pin_op_attrs
- test_reboot_control_attrs
- test_read_only_mfr_attrs
- test_invalid_attrs
- test_threshold_boundary_values
- test_write_protect_attrs
- test_write_protect_logic

ltc4286_alert (21 tests):

- test_disable_vout_alert_sets_bit
- test_enable_vout_alert_clears_bit
- test_disable_iout_alert_sets_bit
- test_enable_iout_alert_clears_bit
- test_disable_input_alert_sets_bit
- test_enable_input_alert_clears_bit
- test_disable_temp_alert_sets_bit
- test_enable_temp_alert_clears_bit
- test_disable_comms_alert_sets_bit
- test_enable_comms_alert_clears_bit
- test_disable_mfr_spec_alert_sets_bit
- test_enable_mfr_spec_alert_clears_bit
- test_disable_status_word_alert_sets_bit
- test_enable_status_word_alert_clears_bit
- test_disable_sys1_alert_sets_bit
- test_enable_sys1_alert_clears_bit
- test_disable_sys2_alert_sets_bit
- test_enable_sys2_alert_clears_bit
- test_alert_toggle
- test_alert_bit_independence
- test_invalid_alert_mask_returns_einval

Emulator Implementation
***********************

The I2C emulator (``ltc4286_emul.c``) simulates:

- 256-byte register map
- I2C read/write operations
- Extended command support (0xFE prefix for 2-byte commands)
- Register persistence across operations
- Little-endian byte ordering
- ``ltc4286_emul_reset()`` to restore state between tests
- ``ltc4286_emul_get_register()`` to inspect register contents directly

Device Configuration
********************

The test uses a default LTC4286 configuration:

- I2C address: 0x40
- Shunt resistor: 10 mohm (10000 kohm)
- Voltage range: 25V mode
- Bus: I2C0 @ 100kHz (standard mode)

References
**********

- `LTC4286 Datasheet <https://www.analog.com/ltc4286>`_
- Zephyr Sensor API: ``include/zephyr/drivers/sensor.h``
- LTC4286 Extended API: ``include/zephyr/drivers/sensor/ltc4286.h``
- LTC4286 Driver: ``drivers/sensor/adi/ltc4286/``
