Power Twister Harness
=====================

A Python testing framework for measuring and validating power consumption in Zephyr RTOS applications using hardware power monitoring probes.

Overview
--------

The Power Twister Harness is designed to automate power consumption testing for embedded devices running Zephyr RTOS. It integrates with hardware power monitoring solutions to measure RMS (Root Mean Square) current values and validate them against expected thresholds with configurable tolerance levels.

Features
--------

* **Hardware Power Monitor Integration**: Supports various power monitoring probes through an abstract interface
* **Multi-Platform Support**: STM Power Shield, General ADC platforms, and extensible architecture
* **RMS Current Measurement**: Calculates RMS current values from raw power data
* **Automated Validation**: Compares measured values against expected thresholds with tolerance checking
* **Peak Detection**: Identifies power consumption transitions and peaks in the measurement data
* **Flexible Configuration**: YAML-based configuration system with environment variable support
* **Real-time Measurements**: Configurable sample rates and measurement durations
* **Comprehensive Logging**: Detailed logging for debugging and result recording

Requirements
------------

* Python 3.8+
* pytest >= 6.0
* PyYAML >= 5.4
* numpy >= 1.20 (for advanced data processing)
* scipy >= 1.7 (for signal processing and peak detection)
* matplotlib >= 3.3 (optional, for plotting)
* Zephyr RTOS testing environment
* Compatible power monitoring hardware

Installation
------------

1. Ensure you have the Zephyr development environment set up
2. Install required Python dependencies::

      pip install pytest PyYAML numpy scipy matplotlib

3. Set up your power monitoring hardware according to the manufacturer's instructions

Supported Hardware Platforms
-----------------------------

STM Power Shield
~~~~~~~~~~~~~~~~

The default power monitoring solution using STM-based hardware.

* High precision current measurements
* Multiple channel support
* USB/Serial interface
* Real-time data streaming

General ADC Platform
~~~~~~~~~~~~~~~~~~~~

A flexible, extensible ADC-based power monitoring solution that works with various hardware platforms and development boards.

**Key Features:**

* **Multi-channel ADC support**: Up to 8 configurable measurement channels
* **YAML-based configuration**: Comprehensive configuration through power_shield.yaml
* **Hardware abstraction**: Compatible with various ADC implementations (SPI, I2C, UART)
* **Advanced calibration**: Built-in scaling, offset correction, and temperature compensation
* **Flexible sampling**: Configurable sample rates from 1Hz to 100kHz
* **Data buffering**: Circular buffer support for continuous measurements
* **Protocol support**: Multiple communication protocols (UART, SPI, I2C, USB)

**Supported ADC Types:**

* External ADC modules (ADS1115, ADS1256, MCP3208, etc.)
* Built-in MCU ADCs (STM32, Nordic nRF, ESP32, etc.)
* Custom ADC implementations via serial protocol
* USB-based ADC devices

**Hardware Connection Examples:**

For current sensing with shunt resistor::

      VDD_SOURCE ──[Rsense]── VDD_TARGET
                  │        │
                  │        └── ADC_CH_NEG (or GND for single-ended)
                  └─────────── ADC_CH_POS
                                  │
      Host PC ──[Serial/USB]── Target Device with ADC

For voltage monitoring::

      VDD_TARGET ──[Voltage Divider]── ADC_CH_INPUT
                                  │
      Host PC ──[Serial/USB]────── Target Device with ADC

See ``general_adc_platform/README.md`` for detailed hardware setup and implementation guide.

Configuration
-------------

Environment Variables
~~~~~~~~~~~~~~~~~~~~~

* ``PROBE_CLASS``: Specifies the power monitor class to use

    - ``stm_powershield`` (default): STM Power Shield
    - ``general_adc_platform.GeneralAdcPowerMonitor.GeneralPowerShield``: General ADC platform

* ``PROBE_SETTING_PATH``: Directory containing probe configuration files (default: current directory)
* ``POWER_SHIELD_CONFIG``: Override default config file name (default: power_shield.yaml)
* ``ADC_DEBUG_MODE``: Enable debug mode for ADC platform (0/1, default: 0)

Configuration Files
~~~~~~~~~~~~~~~~~~~

**power_shield.yaml Example:**

::

    # Power Shield Configuration
    power_shield:
      # Hardware settings
      hardware:
        platform: "general_adc"
        adc_type: "ads1115"
        communication:
          protocol: "uart"
          port: "/dev/ttyUSB0"
          baudrate: 115200
          timeout: 5.0

      # Channel configuration
      channels:
        - id: 0
          name: "VDD_CORE"
          enabled: true
          shunt_resistor: 0.1  # Ohms
          gain: 1
          offset: 0.0
          calibration_factor: 1.0

        - id: 1
          name: "VDD_IO"
          enabled: true
          shunt_resistor: 0.1
          gain: 1
          offset: 0.0
          calibration_factor: 1.0

      # Measurement settings
      measurement:
        sampling_rate: 1000  # Hz
        buffer_size: 10000
        averaging: 1
        resolution: 16  # bits
        reference_voltage: 3.3  # V

      # Processing settings
      processing:
        apply_filtering: true
        filter_type: "lowpass"
        filter_cutoff: 100  # Hz
        remove_dc_offset: true
        calibration_enabled: true

**Configuration Validation:**

The General ADC platform includes comprehensive configuration validation:

* Channel conflict detection
* Hardware capability checking
* Parameter range validation
* Communication protocol verification

Test Data Structure
~~~~~~~~~~~~~~~~~~~

Enhanced test data structure with additional parameters for ST power shield::

      {
          # Basic measurement parameters
          'measurement_duration': 10,          # Duration of measurement in seconds
          'elements_to_trim': 100,             # Initial samples to exclude
          'sampling_rate': 1000,               # ADC sampling rate in Hz

          # Peak detection and analysis
          'num_of_transitions': 5,             # Expected number of power state transitions
          'min_peak_distance': 50,             # Minimum samples between peaks
          'min_peak_height': 0.001,            # Minimum peak height in Amps (1mA)
          'peak_padding': 10,                  # Samples to exclude around peaks
          'peak_detection_algorithm': 'scipy', # 'scipy', 'custom', 'threshold'

          # Expected values and validation
          'expected_rms_values': [1.5, 2.0],   # Expected RMS current values in mA
          'tolerance_percentage': 10,          # Measurement tolerance (±10%)
          'absolute_tolerance': 0.1,           # Absolute tolerance in mA

          # Multi-channel configuration
          'active_channels': [0, 1],           # List of channels to measure
          'channel_weights': [1.0, 1.0],       # Weighting factors for channels
          'synchronize_channels': true,        # Synchronize multi-channel sampling

          # Advanced analysis
          'calculate_power': false,            # Calculate power consumption
          'frequency_analysis': false,         # Perform FFT analysis
          'harmonic_analysis': false,          # Analyze harmonics
          'statistical_analysis': true,        # Calculate statistics

          # Data processing
          'apply_filtering': true,             # Apply digital filtering
          'remove_outliers': true,             # Remove statistical outliers
          'outlier_threshold': 3.0,            # Standard deviations for outlier detection

          # Reporting
          'generate_plots': false,             # Generate measurement plots
          'export_raw_data': false,            # Export raw measurement data
          'detailed_logging': true             # Enable detailed logging
      }

Usage
-----

Basic Test Execution
~~~~~~~~~~~~~~~~~~~~

Using STM Power Shield (default)::

      pytest test_power.py

Using General ADC Platform::

      PROBE_CLASS=general_powershield pytest test_power.py

With Custom Configuration::

      PROBE_SETTING_PATH=/path/to/config PROBE_CLASS=general_powershield pytest test_power.py

Advanced Usage Examples::

      # Enable debug mode
      ADC_DEBUG_MODE=1 PROBE_CLASS=general_powershield pytest test_power.py -v

      # Use custom config file
      POWER_SHIELD_CONFIG=my_adc_config.yaml PROBE_CLASS=general_powershield pytest test_power.py

      # Generate detailed reports
      pytest test_power.py --html=power_report.html --self-contained-html

Outputs
~~~~~~~

A "power_shield" folder will be created in the "build_dir" of DUT application

including::

      handler.log
      <platform>_current_data_<timestamp>.csv
      <platform>_voltage_data_<timestamp>.csv
      <platform>_power_data_<timestamp>.csv

If the csv can successful generated, we judge this as pass criteria. User can analyze
the power number futher.

Hardware Setup Examples
~~~~~~~~~~~~~~~~~~~~~~~

**STM Power Shield Setup:**

::

      Target Device ──[USB]── STM Power Shield ──[USB]── Host PC

**General ADC Platform - UART Setup:**

::

      Target MCU with ADC ──[UART]── Host PC
             │
      Current Sense Circuit:
      VDD ──[Rsense]── Load
             │     │
             │     └── ADC_CH_NEG
             └────── ADC_CH_POS

**General ADC Platform - External ADC Setup:**

::

      Host PC ──[USB/Serial]── MCU ──[SPI/I2C]── External ADC (ADS1115)
                                                       │
                                Current Sense ─────────┘
                                VDD ──[Rsense]── Load

**General ADC Platform - Multi-Channel Setup:**

::

      Host PC ──[USB]── Development Board
                             │
                        ┌────┴────┐
                        │   ADC   │
                        │ CH0-CH3 │
                        └─────────┘
                             │
      ┌─────────────────────────────────────┐
      │ CH0: VDD_CORE  ──[R1]── Load1      │
      │ CH1: VDD_IO    ──[R2]── Load2      │
      │ CH2: VDD_RF    ──[R3]── Load3      │
      │ CH3: VDD_PERIPH──[R4]── Load4      │
      └─────────────────────────────────────┘

Test Function
~~~~~~~~~~~~~

The enhanced test function ``test_power_harness`` performs:

1. **Hardware Detection**: Auto-detect connected ADC hardware
2. **Configuration Loading**: Load and validate YAML configuration
3. **Multi-Channel Setup**: Configure multiple measurement channels
4. **Calibration**: Apply calibration coefficients and corrections
5. **Synchronized Measurement**: Perform synchronized multi-channel sampling
6. **Advanced Processing**: Apply filtering, peak detection, and analysis
7. **Statistical Validation**: Compare against expected values with tolerance
8. **Comprehensive Reporting**: Generate detailed reports and logs

Troubleshooting
---------------

Common Issues
~~~~~~~~~~~~~

**Hardware Connection Issues:**

* Verify USB/Serial connections are secure
* Check device permissions (Linux/macOS may require udev rules)
* Ensure correct COM port or device path in configuration
* Verify power supply to ADC hardware

**Configuration Problems:**

* Validate YAML syntax using online YAML validators
* Check file paths and permissions for configuration files
* Verify channel IDs don't conflict
* Ensure sampling rates are within hardware limits

**Measurement Accuracy:**

* Calibrate shunt resistors with precision multimeter
* Check for ground loops and noise sources
* Verify ADC reference voltage stability
* Use appropriate filtering for noisy environments

**Performance Issues:**

* Reduce sampling rate for long measurements
* Increase buffer sizes for high-speed sampling
* Use appropriate USB cables for high-speed data transfer
* Monitor system resources during measurements

Debug Mode
~~~~~~~~~~

Enable debug mode for detailed troubleshooting::

    ADC_DEBUG_MODE=1 pytest test_power.py -v -s

Debug output includes:

* Hardware detection results
* Configuration validation details
* Real-time measurement data
* Communication protocol debugging
* Error stack traces with context

Logging Configuration
~~~~~~~~~~~~~~~~~~~~~

Configure logging levels in your test environment::

    import logging
    logging.basicConfig(level=logging.DEBUG)

Log levels available:

* ``DEBUG``: Detailed diagnostic information
* ``INFO``: General operational messages
* ``WARNING``: Warning messages for potential issues
* ``ERROR``: Error messages for failures
* ``CRITICAL``: Critical errors that stop execution

Contributing
------------

Development Setup
~~~~~~~~~~~~~~~~~

1. Clone the Zephyr repository
2. Set up development environment::

      cd zephyr/scripts/pylib/power-twister-harness
      pip install -e .
      pip install -r requirements-dev.txt

3. Run tests::

      pytest tests/ -v

Code Style
~~~~~~~~~~

* Follow PEP 8 coding standards
* Use type hints for function signatures
* Add docstrings for all public methods
* Include unit tests for new features

Submitting Changes
~~~~~~~~~~~~~~~~~~

1. Create feature branch from main
2. Implement changes with tests
3. Update documentation as needed
4. Submit pull request with detailed description

License
-------

This project is licensed under the Apache License 2.0. See the LICENSE file in the Zephyr project root for details.

Support
-------

* **Documentation**: https://docs.zephyrproject.org/
* **Issues**: Report bugs and feature requests on GitHub
* **Community**: Join the Zephyr Discord or mailing lists
* **Commercial Support**: Contact Zephyr project maintainers

Changelog
---------

Version 2.0.0
~~~~~~~~~~~~~~

* Added General ADC Platform support
* Multi-channel measurement capabilities
* Enhanced configuration system with YAML validation
* Advanced signal processing and filtering
* Improved error handling and debugging
* Comprehensive test coverage

Version 1.0.0
~~~~~~~~~~~~~~

* Initial release with STM Power Shield support
* Basic RMS current measurement
* Simple peak detection
* pytest integration

API Reference
-------------

Abstract PowerMonitor Interface
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

All power monitor implementations must provide:

* ``connect(dft: DeviceAdapter) -> bool``: Connect to monitoring hardware
* ``init(device_id: str = None) -> bool``: Initialize the power monitor
* ``measure(duration: int) -> None``: Start measurement for specified duration
* ``get_data(duration: int) -> list[float]``: Retrieve measurement data
* ``close() -> None``: Close connection and cleanup resources
