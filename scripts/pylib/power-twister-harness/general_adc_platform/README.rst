General ADC Platform - Power Measurement Shield
===============================================

This directory contains configuration and documentation for the General ADC Platform power measurement system used in Zephyr's power testing harness.

Overview
--------

The General ADC Platform provides a flexible power measurement solution for Zephyr applications. It uses ADC-based current sensing through shunt resistors to monitor power consumption across multiple voltage rails.

Components
----------

GeneralPowerShield
~~~~~~~~~~~~~~~~~~

The GeneralPowerShield is a hardware platform that provides:

- Multi-channel current measurement capability
- Configurable shunt resistor values
- Single-ended and differential measurement modes
- Programmable gain amplification
- Integration with Zephyr's power testing framework

Key Features
~~~~~~~~~~~~

- **Multi-rail monitoring**: Support for monitoring multiple power rails simultaneously
- **High precision**: Low-noise ADC measurements with configurable gain
- **Flexible routing**: Configurable channel mapping for different board layouts
- **Calibration support**: Built-in offset and scale calibration
- **Real-time monitoring**: Integration with power-twister-harness for continuous monitoring

Configuration
-------------

probe_settings.yaml
~~~~~~~~~~~~~~~~~~~

The ``probe_settings.yaml`` file defines the hardware configuration for your specific setup.

Configuration Structure
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

   device_id: "unique_device_identifier"
   routes:
     - id: <route_number>
       name: "<descriptive_name>"
       shunt_resistor: <resistance_in_ohms>
       gain: <amplifier_gain>
       type: "<measurement_type>"
       channels:
         channels_p: <positive_channel_number>
         channels_n: <negative_channel_number>
   calibration:
     offset: <offset_correction>
     scale: <scale_factor>

Configuration Validation
^^^^^^^^^^^^^^^^^^^^^^^^^

The configuration file is validated against a JSON schema (``probe_settings_schema.json``) to ensure correctness and prevent runtime errors.

Schema Validation Rules
"""""""""""""""""""""""

**Required Fields:**

- ``device_id``: Must be a non-empty string
- ``routes``: Must be an array with at least one route
- ``calibration``: Must contain both offset and scale

**Route Validation:**

- ``id``: Integer >= 0, must be unique across routes
- ``name``: Non-empty string describing the power rail
- ``shunt_resistor``: Positive number (> 0) in ohms
- ``gain``: Positive number (> 0) for amplifier gain
- ``type``: Must be either "single" or "differential"
- ``channels.channels_p``: Integer >= 0 for positive channel
- ``channels.channels_n``: Integer >= 0 for negative channel

**Calibration Validation:**

- ``offset``: Any number (can be positive, negative, or zero)
- ``scale``: Positive number (> 0) for scale factor

**Data Types:**

- Numbers can be integers or floating-point values
- Examples: ``0.1``, ``1.0``, ``2.5``, ``100``
     Manual Validation
     """""""""""""""""

     To validate your configuration manually:

     .. code-block:: bash

      # Using the validation script
      python validate_probe_settings.py probe_settings.yaml

      # Or with custom schema file
      python validate_probe_settings.py probe_settings.yaml --schema custom_schema.json

      # For verbose output
      python validate_probe_settings.py probe_settings.yaml --verbose

     Alternative validation using jsonschema directly:

     .. code-block:: bash

      # Using jsonschema (if available)
      python -c "
      import json, yaml
      from jsonschema import validate

      # Load schema and config
      with open('probe_settings_schema.json') as f:
          schema = json.load(f)
      with open('probe_settings.yaml') as f:
          config = yaml.safe_load(f)

      # Validate
      validate(config, schema)
      print('Configuration is valid!')
      "
Common Validation Errors
""""""""""""""""""""""""

**"is less than or equal to the minimum"**

This error occurs when numeric values are not positive where required:

- ``shunt_resistor`` must be > 0 (e.g., use ``0.1`` not ``0``)
- ``gain`` must be > 0 (e.g., use ``1.0`` not ``0``)
- ``scale`` must be > 0 (e.g., use ``1.0`` not ``0``)

**"Missing required property"**

Ensure all required fields are present:

.. code-block:: yaml

   # Correct structure
   device_id: "my_device"
   routes:
     - id: 0
       name: "VDD_CORE"
       shunt_resistor: 0.1
       gain: 1.0
       type: "single"
       channels:
         channels_p: 4
         channels_n: 0
   calibration:
     offset: 0.0
     scale: 1.0

**"Additional properties are not allowed"**

Remove any extra fields not defined in the schema.

Validation Best Practices
"""""""""""""""""""""""""

1. **Use meaningful names**: Route names should clearly identify the power rail
2. **Verify hardware values**: Ensure shunt_resistor matches physical components
3. **Check channel assignments**: Avoid conflicts between routes
4. **Validate ranges**: Ensure gain and scale values are appropriate for your setup
5. **Test configuration**: Validate before deploying to avoid runtime errors

Parameters
^^^^^^^^^^

Device Configuration
"""""""""""""""""""""

- **device_id**: Unique identifier for the measurement device
- **calibration**: Global calibration settings

  - **offset**: Voltage offset correction (V)
  - **scale**: Scale factor for measurements

Route Configuration
"""""""""""""""""""

- **id**: Numeric identifier for the measurement route (0-based)
- **name**: Human-readable name for the power rail (e.g., "VDD_CORE", "VDD_IO")
- **shunt_resistor**: Shunt resistor value in ohms (typically 0.1Ω for low-power measurements)
- **gain**: Amplifier gain setting (1.0 = unity gain)
- **type**: Measurement type

  - ``"single"``: Single-ended measurement
  - ``"differential"``: Differential measurement

- **channels**: ADC channel mapping

  - **channels_p**: Positive input channel number
  - **channels_n**: Negative input channel number (for differential measurements)

Example Configuration
^^^^^^^^^^^^^^^^^^^^^

The provided example monitors two power rails:

1. **VDD_CORE** (Route 0):

   - Single-ended measurement on ADC channel 4
   - 0.1Ω shunt resistor
   - Unity gain

2. **VDD_IO** (Route 1):

   - Single-ended measurement using channels 7 (positive) and 3 (negative)
   - 0.1Ω shunt resistor
   - Unity gain

Usage
-----

1. Hardware Setup
~~~~~~~~~~~~~~~~~

1. Connect your target board to the GeneralPowerShield
2. Ensure proper shunt resistor values are installed
3. Verify ADC channel connections match your configuration

2. Configuration
~~~~~~~~~~~~~~~~

1. Copy and modify ``probe_settings.yaml`` for your specific setup
2. Update device_id, route names, and channel mappings
3. Set appropriate shunt resistor values and gains
4. Perform calibration if needed
5. **Validate configuration** against the schema before use

3. Integration with Power Testing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The configuration is automatically loaded by the power-twister-harness framework:

.. code-block:: python

   # Example usage in test scripts
   from power_twister_harness import GeneralADCPlatform

   # Load configuration (automatically validates)
   platform = GeneralADCPlatform("probe_settings.yaml")

   # Start measurement
   platform.start_measurement()

   # Get power data
   power_data = platform.get_power_readings()

Calibration
-----------

Offset Calibration
~~~~~~~~~~~~~~~~~~

Measure the ADC reading with no current flowing and set the offset to compensate.

Scale Calibration
~~~~~~~~~~~~~~~~~

Apply a known current and adjust the scale factor to match expected readings.

Calibration Procedure
~~~~~~~~~~~~~~~~~~~~~

1. Connect a precision current source
2. Record ADC readings at multiple current levels
3. Calculate offset and scale corrections
4. Update ``probe_settings.yaml`` with calibration values
5. **Re-validate configuration** after changes

Troubleshooting
---------------

Common Issues
~~~~~~~~~~~~~

1. **Incorrect readings**: Check shunt resistor values and channel mappings
2. **Noisy measurements**: Verify grounding and reduce gain if necessary
3. **Offset drift**: Perform regular calibration, especially after temperature changes
4. **Channel conflicts**: Ensure each route uses unique ADC channels
5. **Configuration validation errors**: Check schema compliance and data types

Verification Steps
~~~~~~~~~~~~~~~~~~

1. **Validate configuration file** against schema
2. Verify hardware connections match configuration
3. Check shunt resistor values with multimeter
4. Validate ADC channel assignments
5. Test with known current loads

Hardware Requirements
---------------------

- GeneralPowerShield board
- Compatible ADC (typically 16-bit or higher resolution)
- Precision shunt resistors (0.1Ω typical)
- Target board with accessible power rails
- USB or serial connection for data acquisition

Software Dependencies
---------------------

- Python 3.7+
- Zephyr power-twister-harness framework
- ADC driver libraries
- YAML configuration parser
- JSON Schema validation library (jsonschema)

Contributing
------------

When modifying configurations:

1. **Validate against schema** before committing
2. Test thoroughly with known loads
3. Document any hardware changes
4. Update calibration values after hardware modifications
5. Validate measurements against reference equipment
6. Update schema if adding new configuration options

Support
-------

For issues related to:

- **Hardware setup**: Check connections and component values
- **Configuration**: Validate YAML syntax and parameter ranges
- **Schema validation**: Check data types and required fields
- **Calibration**: Use precision reference equipment
- **Integration**: Refer to power-twister-harness documentation
