.. _twister_power_harness:

Power
#####

The ``power`` harness is used to measure and validate the current consumption.
It integrates with 'pytest' to perform automated data collection and analysis using a hardware power monitor.

The harness executes the following steps:

1. Initializes a power monitoring device (e.g., ``stm_powershield``) via the ``PowerMonitor`` abstract interface.
#. Starts current measurement for a defined ``measurement_duration``.
#. Collects raw current waveform data.
#. Uses a peak detection algorithm to segment data into defined execution phases based on power transitions.
#. Computes RMS current values for each phase using a utility function.
#. Compares the computed values with user-defined expected RMS values.

.. code-block:: yaml

    harness: power
    harness_config:
      fixture: pm_probe
      power_measurements:
        elements_to_trim: 100
        min_peak_distance: 40
        min_peak_height: 0.008
        peak_padding: 40
        measurement_duration: 6
        num_of_transitions: 4
        expected_rms_values: [56.0, 4.0, 1.2, 0.26, 140]
        tolerance_percentage: 20

- **elements_to_trim** – Number of samples to discard at the start of measurement to eliminate noise.
- **min_peak_distance** – Minimum distance between detected current peaks (helps detect distinct transitions).
- **min_peak_height** – Minimum current threshold to qualify as a peak (in amps).
- **peak_padding** – Number of samples to extend around each detected peak.
- **measurement_duration** – Total time (in seconds) to record current data.
- **num_of_transitions** – Expected number of power state transitions in the DUT during test execution.
- **expected_rms_values** – Target RMS values for each identified execution phase (in milliamps).
- **tolerance_percentage** – Allowed deviation percentage from the expected RMS values.
