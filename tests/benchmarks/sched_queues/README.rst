Scheduling Queue Measurements
#############################

A Zephyr application developer may choose between three different scheduling
algorithms: dumb, scalable and multiq. These different algorithms have
different performance characteristics that vary as the
number of ready threads increases. This benchmark can be used to help
determine which scheduling algorithm may best suit the developer's application.

This benchmark measures:

* Time to add a threads of increasing priority to the ready queue.
* Time to add threads of decreasing priority to the ready queue.
* Time to remove highest priority thread from a wait queue.
* Time to remove lowest priority thread from a wait queue.

By default, these tests show the minimum, maximum, and averages of the measured
times. However, if the verbose option is enabled then the set of measured
times will be displayed. The following will build this project with verbose
support:

.. code-block:: shell

    EXTRA_CONF_FILE="prj.verbose.conf" west build -p -b <board> <path to project>

Alternative output with ``CONFIG_BENCHMARK_RECORDING=y`` is to show the measured
summary statistics as records to allow Twister parse the log and save that data
into ``recording.csv`` files and ``twister.json`` report.
This output mode can be used together with the verbose output, however only
the summary statistics will be parsed as data records.
