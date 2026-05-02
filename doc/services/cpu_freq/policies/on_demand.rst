.. _on_demand_policy:

On-Demand CPU Frequency Scaling Policy
######################################

The On-Demand policy evaluates the current CPU load using the
:ref:`CPU Load metric <cpu_load_metric>`, and compares it to the trigger threshold defined by the
SoC P-state definition.

The On-Demand policy will iterate through the defined P-states and select the first P-state for which
the CPU load is greater than or equal to the defined threshold.

If no P-state matches (i.e., the CPU load is below all thresholds), the policy will select the
last P-state in the soc_pstates array (the lowest performance state). This is intrinsic to the
policy:  P-states must be defined in decreasing threshold order in devicetree, and the last
P-state will be used for loads below all thresholds.

For an example of the on-demand policy refer to the :zephyr:code-sample:`cpu_freq_on_demand` sample.

This policy is reactive. Frequency adjustments occur only after a change in system load has been
observed, so it cannot anticipate sudden high loads. The policy has no notion of task deadlines and
should not be considered as a real-time policy.
