.. _external_module_zview:

ZView
#####

Introduction
************

`ZView <zview_>`_ is a runtime visualizer for Zephyr RTOS applications, providing live system-wide
thread and heap statistics via an SWD debug probe.

It reads kernel object locations and inspects memory through the APB bus 'without halting' the CPU,
keeping the on-target footprint to nearly zero — no UART, no Shell, no additional Kconfig overhead
beyond the standard thread introspection options.

The tool runs entirely on the host as a TUI application, displaying live stack watermarks,
CPU usage per thread, and heap runtime statistics.

Usage with Zephyr
*****************

Declare the module in your workspace manifest, or pull it in via a submanifest.
For example, create ``zephyrproject/zephyr/submanifests/zview.yaml`` with the following content:

.. code-block:: yaml

   manifest:
     projects:
       - name: zview
         url: https://github.com/wkhadgar/zview
         revision: main
         path: modules/tools/zview
         west-commands: scripts/west-commands.yml

Your application must be compiled and running with the appropriate Kconfig options. At minimum:

.. code-block:: cfg

CONFIG_INIT_STACKS=y
CONFIG_THREAD_MONITOR=y
CONFIG_THREAD_STACK_INFO=y

Then update the workspace and run ZView through the integrated west command:

.. code-block:: sh

   west update
   west zview

Refer to the `ZView repository <zview_>`_ for the full list of supported options and CLI usage.

Reference
*********

- `ZView repository <zview_>`_

.. _zview: https://github.com/wkhadgar/zview
