.. _tracing:

Tracing
#######

Overview
********

The tracing feature provides hooks that permits you to collect data from
your application and allows enabled backends to visualize the inner-working of
the kernel and various subsystems.

Applications and supported tools can define empty macros declared in
:file:`include/tracing.h` that are called across the kernel in key spots.


SEGGER SystemView Support
*************************

Zephyr provides built-in support for `SEGGER SystemView`_ that can be enabled in
any application for platforms that have the required hardware support.

To enable tracing support with `SEGGER SystemView`_ add the configuration option
:option:`CONFIG_SEGGER_SYSTEMVIEW` to your project configuration file and set
it to *y*. For example, this can be added to the
:ref:`dining-philosophers-sample` to visualize fast switching between threads.

.. _SEGGER SystemView: https://www.segger.com/products/development-tools/systemview/


