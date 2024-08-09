.. _profiling-perf:


Perf
####


Perf is a profiler tool, based on stack tracing. It can be used to light weight profiling
almost without any code overhead.

Work principe
*************

``perf record`` shell-command starts timer with perf tracer function.
Timers are driven by inerrupts, so perf tracer function is calling during interruption.
Zephyr core save return address and frame pointer in interrupt stack or ``callee_saved``
struture before caling interrupt hundler. So perf trace function makes stack traces by
using return address and frame pointer.

Then stackcollapse.py script convert return addresses in stack trace to function names
by elf file and print them in required for FlameGraph format.

Configuration
*************

Configure this module using the following options.

* :kconfig:option:`CONFIG_PROFILING_PERF`: enable the module. This opion add
  perf command in shell.

* :kconfig:option:`CONFIG_PROFILING_PERF_BUFFER_SIZE`: sets size of perf buffer,
  where samples are saved before printing.

Usage
*****

More specific example can be found in :zephyr:code-sample:`profiling-perf`.

#. Build with perf enabled and run.

#. Begin perf sampling by shell-command

   .. code-block:: text

      perf record <period> <frequency>

   This command will start timer with perf sampler for *period* in seconds and
   with *frequency* in hertz.

   Wait for the completion message ``perf done!`` or ``perf buf override!``
   (if perf buffer size is smaller than required).

#. Print made by perf samles in terminal by shell-command

   .. code-block:: text

      perf printbuf

   Output exaple:

   .. code-block:: text

      Perf buf length 1985
      0000000000000004
      0000000080004e58
      00000000800071ca
      0000000080000964
      aaaaaaaaaaaaaaaa
      0000000000000002
      ....
      00000000800163e0

   Copy gotten output in some file, for example *perf_buf*.

#. Translate perf samples by stackcollapse script to format, required by FlameGraph

   .. code-block:: bash

      python zephyr/scripts/perf/stackcollapse.py perf_buf <build_dir>/zephyr/zephyr.elf > perf_buf.folded

   And turn into .svg FlameGraph (for example *graph.svg*)

   .. code-block:: bash

      ./FlameGraph/flamegraph.pl perf_buf.folded > graph.svg

   All this step can be done by one command

   .. code-block:: bash

      python zephyr/scripts/perf/stackcollapse.py perf_buf <build_dir>/zephyr/zephyr.elf | ./FlameGraph/flamegraph.pl > graph.svg
