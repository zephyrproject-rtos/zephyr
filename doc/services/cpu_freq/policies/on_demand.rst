.. _on_demand_policy:

On-Demand CPU Frequency Scaling Policy
######################################

The On-Demand policy evaluates the current CPU load using the :ref:`CPU Load subsystem <cpu_load_subsys>`,
and compares it to the trigger threshold defined by the SoC P-state definition.

The On-Demand policy will iterate through the defined P-states and select the first P-state of which the
CPU load exceeds the defined threshold.

For an example of the on-demand policy refer to the :zephyr:code-sample:`cpu_freq_on_demand` sample.
