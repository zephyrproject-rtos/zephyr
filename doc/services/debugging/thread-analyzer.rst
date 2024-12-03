.. _thread_analyzer:

Thread analyzer
###################

The thread analyzer module enables all the Zephyr options required to track
the thread information, e.g. thread stack size usage and other runtime thread
runtime statistics.

The analysis is performed on demand when the application calls
:c:func:`thread_analyzer_run` or :c:func:`thread_analyzer_print`.

For example, to build the synchronization sample with Thread Analyser enabled,
do the following:

   .. zephyr-app-commands::
      :zephyr-app: samples/synchronization/
      :board: qemu_x86
      :goals: build
      :gen-args: -DCONFIG_QEMU_ICOUNT=n -DCONFIG_THREAD_ANALYZER=y \
                   -DCONFIG_THREAD_ANALYZER_USE_PRINTK=y -DCONFIG_THREAD_ANALYZER_AUTO=y \
                   -DCONFIG_THREAD_ANALYZER_AUTO_INTERVAL=5


When you run the generated application in Qemu, you will get the additional
information from Thread Analyzer::


	thread_a: Hello World from cpu 0 on qemu_x86!
	Thread analyze:
	 thread_b            : STACK: unused 740 usage 284 / 1024 (27 %); CPU: 0 %
	 thread_analyzer     : STACK: unused 8 usage 504 / 512 (98 %); CPU: 0 %
	 thread_a            : STACK: unused 648 usage 376 / 1024 (36 %); CPU: 98 %
	 idle                : STACK: unused 204 usage 116 / 320 (36 %); CPU: 0 %
	thread_b: Hello World from cpu 0 on qemu_x86!
	thread_a: Hello World from cpu 0 on qemu_x86!
	thread_b: Hello World from cpu 0 on qemu_x86!
	thread_a: Hello World from cpu 0 on qemu_x86!
	thread_b: Hello World from cpu 0 on qemu_x86!
	thread_a: Hello World from cpu 0 on qemu_x86!
	thread_b: Hello World from cpu 0 on qemu_x86!
	thread_a: Hello World from cpu 0 on qemu_x86!
	Thread analyze:
	 thread_b            : STACK: unused 648 usage 376 / 1024 (36 %); CPU: 7 %
	 thread_analyzer     : STACK: unused 8 usage 504 / 512 (98 %); CPU: 0 %
	 thread_a            : STACK: unused 648 usage 376 / 1024 (36 %); CPU: 9 %
	 idle                : STACK: unused 204 usage 116 / 320 (36 %); CPU: 82 %
	thread_b: Hello World from cpu 0 on qemu_x86!
	thread_a: Hello World from cpu 0 on qemu_x86!
	thread_b: Hello World from cpu 0 on qemu_x86!
	thread_a: Hello World from cpu 0 on qemu_x86!
	thread_b: Hello World from cpu 0 on qemu_x86!
	thread_a: Hello World from cpu 0 on qemu_x86!
	thread_b: Hello World from cpu 0 on qemu_x86!
	thread_a: Hello World from cpu 0 on qemu_x86!
	Thread analyze:
	 thread_b            : STACK: unused 648 usage 376 / 1024 (36 %); CPU: 7 %
	 thread_analyzer     : STACK: unused 8 usage 504 / 512 (98 %); CPU: 0 %
	 thread_a            : STACK: unused 648 usage 376 / 1024 (36 %); CPU: 8 %
	 idle                : STACK: unused 204 usage 116 / 320 (36 %); CPU: 83 %
	thread_b: Hello World from cpu 0 on qemu_x86!
	thread_a: Hello World from cpu 0 on qemu_x86!
	thread_b: Hello World from cpu 0 on qemu_x86!


Configuration
*************
Configure this module using the following options.

:kconfig:option:`CONFIG_THREAD_ANALYZER`
   Enable the module.
:kconfig:option:`CONFIG_THREAD_ANALYZER_USE_PRINTK`
   Use printk for thread statistics.
:kconfig:option:`CONFIG_THREAD_ANALYZER_USE_LOG`
   Use the logger for thread statistics.
:kconfig:option:`CONFIG_THREAD_ANALYZER_AUTO`
   Run the thread analyzer automatically.
   You do not need to add any code to the application when using this option.
:kconfig:option:`CONFIG_THREAD_ANALYZER_AUTO_INTERVAL`
   The time for which the module sleeps between consecutive printing of thread analysis in automatic
   mode.
:kconfig:option:`CONFIG_THREAD_ANALYZER_AUTO_STACK_SIZE`
  The stack for thread analyzer automatic thread.
:kconfig:option:`CONFIG_THREAD_NAME`
  Print the name of the thread instead of its ID.
:kconfig:option:`CONFIG_THREAD_RUNTIME_STATS`
  Print thread runtime data such as utilization.
  This options is automatically selected by :kconfig:option:`CONFIG_THREAD_ANALYZER`.

API documentation
*****************

.. doxygengroup:: thread_analyzer
