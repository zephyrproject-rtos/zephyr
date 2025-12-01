.. zephyr:code-sample:: nrf_clock_control
   :name: nRF clock control

    Request minimum clock requirements at runtime.

Demonstrates how to request minimum clock requirements of nordic clocks, including getting the
startup time to ensure clock requirements are met in time.

Requirements
************

Requires an nRF54H20 based board.

Building, Flashing and Running
******************************

The sample is built to test a specific clock requirement for a specific clock. The clock is
specified in a devicetree overlay, and the clock requirement is defined using these sample
specific Kconfigs:

* :kconfig:option:`CONFIG_SAMPLE_CLOCK_FREQUENCY_HZ`
* :kconfig:option:`CONFIG_SAMPLE_CLOCK_ACCURACY_PPM`
* :kconfig:option:`CONFIG_SAMPLE_CLOCK_PRECISION`

With the following controlling how long the request is held:

* :kconfig:option:`CONFIG_SAMPLE_PRE_REQUEST_TIMEOUT`
* :kconfig:option:`CONFIG_SAMPLE_KEEP_REQUEST_TIMEOUT`

Example configs and overlays are added for every clock, which can be found in :file:`sample.yaml`,
and applied using the ``-T`` west argument. The following example builds the sample to test the
FLL16M:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nordic/clock_control
   :board: nrf54h20dk/nrf54h20/cpuapp
   :goals: build
   :west-args: -T sample.boards.nrf.clock_control.fll16m
   :compact:

Example output:

.. code-block:: none

   clock name: fll16m
   minimum frequency request: 0Hz
   minimum accuracy request: 30PPM
   minimum precision request: 0

   resolved frequency request: 16000000Hz
   resolved accuracy request: 30PPM
   resolved precision request: 0

   startup time for requested spec: 850us

   requesting minimum clock specs
   request applied to clock in 1ms

   releasing requested clock specs
   clock spec request released
