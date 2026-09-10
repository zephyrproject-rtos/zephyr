Timeout Management Measurements
###############################

Zephyr application developers may need to characterize the performance of the
timeout management system for their platform. This benchmark provides a set of
tests that measure the time taken to add and remove timeouts from the system.

This benchmark measures:

* Time to add timeouts with increasing timeout values
* Time to add timeouts with decreasing timeout values
* Time to remove timeouts from earliest expiration to latest
* Time to remove timeouts from latest expiration to earliest

By default, these tests show the minimum, maximum, and averages of the measured
times. However, if the verbose option is enabled then the set of measured
times will be displayed. The following will build this project with verbose
support:

.. code-block:: shell

    EXTRA_CONF_FILE="prj.verbose.conf" west build -p -b <board> <path to project>
