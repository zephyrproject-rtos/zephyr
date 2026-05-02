.. _cpu_load_metric:

CPU Load
########

The CPU Load metric returns the CPU load as a percentage for the current CPU, since the last time
it was called.

The ``cpu_load_*`` APIs include CPU runtime-history samples. New users should enable
:kconfig:option:`CONFIG_CPU_LOAD_METRIC`.

For an example of the CPU Load metric refer to :zephyr:code-sample:`cpu_freq_on_demand` sample.
