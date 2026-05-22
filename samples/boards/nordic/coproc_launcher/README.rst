.. zephyr:code-sample:: coproc_launcher
   :name: Nordic coprocessor launcher

   Launch coprocessor.

Overview
********

This sample allocates all peripherals to, and starts a specific nordic coprocessor. Board specific
overlays can be added to deallocate all peripherals not needed to boot the coprocessors. The sample
is a secondary image, included if SB_CONFIG_NORDIC_COPROC_LAUNCHER=y and the build target is a
supported nordic coprocessor.

The sample reuses the required dtc overlays and conf files applied by the nordic snippets for
launching the coprocessor.

Building and Running
********************

This nordic coprocessor launcher is included by default by sysbuild when building for a nordic
coprocessor:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: nrf54h20dk/nrf54h20/cpurad
   :goals: run
   :gen-args: --sysbuild
   :compact:

The hello_world sample will be built for the nrf54h20 cpurad coprocessor, and the
nordic coprocessor launcher will be built for the nrf54h20 cpuapp processor.
