.. zephyr:code-sample:: profiling-perf
   :name: Perf tool

    Send perf samples to the host with console support

This application can be used to demonstrate :ref:`profiling-perf` work.

Requirements
************

Perf tool so far implemente only for riscv and x86_64 architectures.

Usage example
*************

Build an image with:

.. zephyr-app-commands::
    :zephyr-app: samples/subsys/profiling/perf
    :board: qemu_riscv64
    :goals: build
    :compact:

After the sample starts, enter shell command:

.. code-block:: console

  uart:~$ perf record <period> <frequency>

Wait for the completion message ``perf done!`` or ``perf buf override!``
(if perf buffer size is smaller than required).

Print made by perf samles in terminal by shell-command:

.. code-block:: console

  uart:~$ perf printbuf

Output exaple:

.. code-block:: console

    Perf buf length 2046
    0000000000000004
    00000000001056b2
    0000000000108192
    000000000010052f
    0000000000000000
      ....
    000000000010052f
    0000000000000000

Copy gotten output in some file, for example *perf_buf*.

Make graph.svg with :zephyr_file:`scripts/perf/stackcollapse.py` and
`FlameGraph`_:

.. _FlameGraph: https://github.com/brendangregg/FlameGraph/

.. code-block:: shell

  python scripts/perf/stackcollapse.py perf_buf build/zephyr/zephyr.elf | <flamegraph_dir_path>/flamegraph.pl > graph.svg

Graph example
=============

.. image:: images/graph_example.svg
    :align: center
    :alt: graph example
