.. zephyr:code-sample:: cpu_freq_thermal_cap
   :name: CPU frequency thermal cap

   Constrain CPU frequency from die temperature.

Overview
********

This sample demonstrates the :ref:`CPU frequency thermal cap <cpu_freq_thermal_cap>`.
The on-demand CPU frequency policy selects the requested P-state from CPU load, while
the thermal cap limits the highest-performance P-state allowed by die temperature.

The board overlay enables the board die temperature sensor and configures low thermal
trip points so the cap behavior is easy to observe during the sample. Product firmware
should choose trip points for its own thermal design.

The sample does not automatically cycle the die temperature through the configured
range. To observe all trip points, use a controlled external heat source to warm the
board and let it cool back below the release thresholds.

Building and Running
********************

Build and run this sample as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/cpu_freq/thermal_cap
   :board: frdm_mcxn236
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: none

   CPUFreq thermal cap sample
   trip 0: 25.000 C -> cap pstate-1, release at 22.000 C
   trip 1: 40.000 C -> cap pstate-2, release at 37.000 C

   die temperature: 24.875 C, thermal cap: none
   die temperature: 25.125 C, thermal cap: pstate-1
   die temperature: 40.250 C, thermal cap: pstate-2
