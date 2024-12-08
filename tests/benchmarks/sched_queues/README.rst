Scheduling Queue Measurements
#############################

A Zephyr application developer may choose between three different scheduling
algorithms--dumb, scalable and multiq. These different algorithms have
different performance characteristics--characteristics that vary as the
number of ready threads increases. This benchmark can be used to help
determine which scheduling algorithm may best suit the developer's application.

This benchmark measures the ...
* Time to add a threads of increasing priority to the ready queue
* Time to add threads of decreasing priority to the ready queue
* Time to remove highest priority thread from a wait queue
* Time to remove lowest priority thread from a wait queue

By default, these tests show the minimum, maximum, and averages of the measured
times. However, if the verbose option is enabled then the set of measured
times will be displayed. The following will build this project with verbose
support:

    EXTRA_CONF_FILE="prj.verbose.conf" west build -p -b <board> <path to project>
