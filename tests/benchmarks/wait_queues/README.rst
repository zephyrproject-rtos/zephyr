Wait Queue Measurements
#######################

A Zehpyr application developer may choose between two different wait queue
implementations: dumb and scalable. These two queue implementations perform
differently under different loads. This benchmark can be used to showcase how
the performance of these two implementations vary under varying conditions.

These conditions include:

* Time to add threads of increasing priority to a wait queue
* Time to add threads of decreasing priority to a wait queue
* Time to remove highest priority thread from a wait queue
* Time to remove lowest priority thread from a wait queue

By default, these tests show the minimum, maximum, and averages of the measured
times. However, if the verbose option is enabled then the raw timings will also
be displayed. The following will build this project with verbose support:

.. code-block:: shell

    EXTRA_CONF_FILE="prj.verbose.conf" west build -p -b <board> <path to project>

Alternative output with ``CONFIG_BENCHMARK_RECORDING=y`` is to show the measured
summary statistics as records to allow Twister parse the log and save that data
into ``recording.csv`` files and ``twister.json`` report.
This output mode can be used together with the verbose output, however only
the summary statistics will be parsed as data records.
