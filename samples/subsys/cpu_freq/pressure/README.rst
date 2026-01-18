.. zephyr:code-sample:: cpu_freq_pressure
   :name: Pressure based CPU frequency scaling

   Dynamically scale CPU frequency using the pressure policy.

Overview
********

This sample demonstrates the :ref:`CPU frequency subsystem's <cpu_freq>` pressure policy. The
sample will print debug information like system pressure and CPU frequency subsystem logs to the
console for visibility into the subsystem.

The example will iterate through some simulated CPU threading scenarios in the main application.
