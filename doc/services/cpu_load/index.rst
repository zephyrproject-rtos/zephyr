.. _cpu_load:

CPU load
########

The CPU load module tracks the fraction of time the CPU spends outside of the idle state. Two
measurement backends are available, selected with the ``CPU_LOAD_BACKEND`` Kconfig choice:

Scheduler runtime statistics
   :kconfig:option:`CONFIG_CPU_LOAD_BACKEND_RUNTIME_STATS` derives the load from the scheduler
   per-CPU runtime statistics. It is portable across architectures and supports multiple CPUs.

Architecture idle hooks
   :kconfig:option:`CONFIG_CPU_LOAD_BACKEND_IDLE_HOOK` measures the load using the architecture
   idle hooks, which are called before and after the CPU goes to idle. It has lower overhead than
   the runtime-statistics backend and can use a :ref:`counter_api` device for higher precision
   (the counter path is single-CPU only). It is available on any architecture that emits the idle
   hooks (:kconfig:option:`CONFIG_ARCH_HAS_CPU_IDLE_HOOKS`). Compared to :ref:`thread_analyzer` it
   is more accurate since it takes into account time spent in the interrupt context as well. This
   backend does not depend on the tracing subsystem.

The load is retrieved with :c:func:`cpu_load_get` for the current CPU or :c:func:`cpu_load_get_cpu`
for a specific CPU. Both return the load in per mille (0...1000) and can reset the measurement
window. Use :c:macro:`CPU_LOAD_PERMILLE_TO_PERCENT` to convert the value to whole percent.

The load can also be reported periodically using a logging message. The period is configured using
:kconfig:option:`CONFIG_CPU_LOAD_LOG_PERIODICALLY`.

For an example refer to the :zephyr:code-sample:`cpu_freq_on_demand` sample.

Using a counter device
**********************

The idle-hook backend uses :c:func:`k_cycle_get_32` by default. When higher precision is needed a
:ref:`counter_api` device can be used by enabling :kconfig:option:`CONFIG_CPU_LOAD_USE_COUNTER` and
setting the chosen node in devicetree.

.. code-block:: devicetree

   chosen {
     zephyr,cpu-load-counter = &counter_device;
   };
