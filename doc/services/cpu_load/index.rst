.. _cpu_load_subsys:

CPU Load
########

The CPU Load subsystem makes use of the ``usage`` kernel feature to implement
:c:func:`cpu_load_get()` which returns the CPU load as a percentage for the current CPU, since the
last time it was called.

The CPU load is calculated by taking the total amount of cycles the CPU spent outside of the idle
thread and dividing it by the total cycle count of the CPU.

This getter should be called on a periodic basis as to ensure that the CPU load is accurately
represented over time.

For an example of the CPU Load subsystem refer to :zephyr:code-sample:`cpu_freq_on_demand` sample.
