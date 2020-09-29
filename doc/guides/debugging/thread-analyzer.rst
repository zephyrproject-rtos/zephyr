.. _thread_analyzer:

Thread analyzer
###################

The thread analyzer module enables all the Zephyr options required to track
the thread information, e.g. thread stack size usage.
The analysis is performed on demand when the application calls
:c:func:`thread_analyzer_run` or :c:func:`thread_analyzer_print`.

Configuration
*************
Configure this module using the following options.

* ``THREAD_ANALYZER``: enable the module.
* ``THREAD_ANALYZER_USE_PRINTK``: use printk for thread statistics.
* ``THREAD_ANALYZER_USE_LOG``: use the logger for thread statistics.
* ``THREAD_ANALYZER_AUTO``: run the thread analyzer automatically.
  You do not need to add any code to the application when using this option.
* ``THREAD_ANALYZER_AUTO_INTERVAL``: the time for which the module sleeps
  between consecutive printing of thread analysis in automatic mode.
* ``THREAD_ANALYZER_AUTO_STACK_SIZE``: the stack for thread analyzer
  automatic thread.
* ``THREAD_NAME``: enable this option in the kernel to print the name of the
  thread instead of its ID.

API documentation
*****************

.. doxygengroup:: thread_analyzer
   :project: Zephyr
