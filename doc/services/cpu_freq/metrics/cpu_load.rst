.. _cpu_load_metric:

CPU Load Metric
###############

The CPU Load metric makes use of :ref:`thread_analyzer` to implement, ``get_cpu_load(uint32_t *load)``
which returns the CPU load as a percentage for the current CPU, since the last time it was called.

The CPU load is calculated by taking the total amount of cycles the CPU spent outside of the idle thread
and dividing it by the total cycle count of the CPU.

This metric should be called on a constant, periodic basis as to ensure that the CPU load is accurately
represented over time.

For an example of the settings subsystem refer to :zephyr:code-sample:`cpu_freq_on_demand` sample.
