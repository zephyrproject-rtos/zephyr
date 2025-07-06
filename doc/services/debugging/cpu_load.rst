.. _cpu_load:

CPU load
########

Module can be used to track how much time is spent in idle. It is using tracing hooks
which are called before and after CPU goes to idle. Compared to :ref:`thread_analyzer`
it is more accurate since it takes into account time spent in the interrupt context as well.

Function :c:func:`cpu_load_get` is used to get the latest value. It is also used to reset
the measurement. By default, module is using :c:func:`k_cycle_get_32` but in cases when higher
precision is needed a :ref:`counter_api` device can be used.

Load can also be reported periodically using a logging message. Period is configured using :kconfig:option:`CONFIG_CPU_LOAD_LOG_PERIODICALLY`.

Using counter device
********************

In order to use :ref:`counter_api` device :kconfig:option:`CONFIG_CPU_LOAD_USE_COUNTER` must be
enabled and chosen in devicetree must be set.

.. code-block:: devicetree

   chosen {
     zephyr,cpu-load-counter = &counter_device;
   };
