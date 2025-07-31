.. _adc_sample:

ADC Sample
##########

Overview
********

This sample demonstrates the ADC (Analog-to-Digital Converter) functionality on the
LPC54S018 board. The application shows how to configure and read multiple ADC channels,
displaying both raw ADC values and converted voltage measurements.

The sample uses the LPC54S018's LPADC (Low-Power ADC) which features:

- 12-bit resolution
- Multiple input channels
- Internal voltage reference
- Configurable sampling rate

The sample demonstrates:

- Configuring multiple ADC channels
- Reading ADC values from channels 0, 4, and 5
- Converting raw ADC values to millivolts
- Periodic sampling with LED indication

Requirements
************

- LPC54S018 board
- Analog voltage sources to measure (0-3.3V range)
- LED for visual feedback

Hardware Connections
********************

The ADC channels are mapped to the following pins:

- Channel 0: P0.10 (ADC0_0)
- Channel 4: P0.16 (ADC0_4) 
- Channel 5: P0.31 (ADC0_5)
- Channel 6: P1.0  (ADC0_6)
- Channel 7: P2.0  (ADC0_7)
- Channel 8: P2.1  (ADC0_8)

.. warning::
   Do not apply voltages outside the 0-3.3V range to the ADC inputs.

Building and Running
********************

Build and flash this sample as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/adc
   :board: lpcxpresso54s018
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v3.7.0 ***
   [00:00:00.010,000] <inf> adc_sample: ADC Sample Application
   [00:00:00.010,000] <inf> adc_sample: Board: lpcxpresso54s018
   [00:00:00.020,000] <inf> adc_sample: ADC device is ready
   [00:00:00.020,000] <inf> adc_sample: Configuring ADC channels...
   [00:00:00.030,000] <inf> adc_sample: ADC sampling:
   [00:00:00.030,000] <inf> adc_sample: - Channel 0: 2048 raw
   [00:00:00.030,000] <inf> adc_sample:   1650 mV
   [00:00:00.040,000] <inf> adc_sample: - Channel 4: 1024 raw
   [00:00:00.040,000] <inf> adc_sample:   825 mV
   [00:00:00.050,000] <inf> adc_sample: - Channel 5: 3072 raw
   [00:00:00.050,000] <inf> adc_sample:   2475 mV
   [00:00:00.050,000] <inf> adc_sample: ---
   [00:00:02.050,000] <inf> adc_sample: ADC sampling:
   [00:00:02.050,000] <inf> adc_sample: - Channel 0: 2050 raw
   [00:00:02.050,000] <inf> adc_sample:   1652 mV

Testing
*******

To test the ADC functionality:

1. Connect a variable voltage source (e.g., potentiometer) to one of the ADC input pins
2. Ensure the voltage is within the 0-3.3V range
3. Observe the ADC readings change as you vary the input voltage
4. The LED toggles with each sampling cycle

ADC Configuration
=================

The sample configures the ADC with:

- 12-bit resolution (0-4095 raw value range)
- Internal voltage reference (3.3V)
- Gain of 1
- Default acquisition time

The voltage calculation uses:

.. code-block:: c

   voltage_mv = (raw_value * 3300) / 4096

References
**********

- :ref:`adc_api`
- `LPC54S018 Reference Manual`_ (Chapter on ADC)

.. _LPC54S018 Reference Manual:
   https://www.nxp.com/docs/en/reference-manual/LPC54S01XJ2J4RM.pdf