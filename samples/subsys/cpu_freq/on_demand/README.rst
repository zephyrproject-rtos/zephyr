.. zephyr:code-sample:: cpu_freq_on_demand
   :name: On-demand CPU frequency scaling

   Dynamically scale CPU frequency using the on-demand policy.

Overview
********

This sample demonstrates the :ref:`CPU frequency subsystem's <cpu_freq>` on-demand policy. The
sample will print debug information like CPU load and CPU frequency subsystem logs to the
console for visibility into the subsystem.

The example will iterate through some simulated CPU processing scenarios in the main application.
