.. _on_demand_policy:

On-Demand CPU Frequency Scaling Policy
######################################

The On-Demand policy evaluates the current CPU load using the :ref:`CPU Load metric <cpu_load_metric>`,
and compares it to the trigger threshold defined by the SoC P-state definition.

The On-Demand policy will iterate through the defined P-states and select the first P-state of which the
CPU load exceeds the defined threshold.

For an example of the settings subsystem refer to :zephyr:code-sample:`cpu_freq_on_demand` sample.
