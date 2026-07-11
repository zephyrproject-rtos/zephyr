.. _cpu_load_metric:

CPU Load Metric
###############

The CPU Load metric returns the CPU load as a percentage for the current CPU, since the last time
it was called. It is derived from the scheduler per-CPU runtime statistics, so it is available on
all architectures and supports multiple CPUs.

The metric is retrieved with :c:func:`cpu_load_metric_get`, declared in
:zephyr_file:`include/zephyr/sys/cpu_load_metric.h` and enabled with
:kconfig:option:`CONFIG_CPU_LOAD_METRIC`.

For an example of the CPU Load metric refer to :zephyr:code-sample:`cpu_freq_on_demand` sample.
