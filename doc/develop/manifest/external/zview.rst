.. _external_module_zview:

ZView
#####

Introduction
************

`ZView <zview_>`_ is a runtime visualizer for Zephyr RTOS applications, providing live system-wide
thread and kernel object statistics via an SWD debug probe.

It reads kernel object locations and inspects memory through the APB bus 'without halting' the CPU,
keeping the on-target footprint to nearly zero — no UART, no Shell, no additional Kconfig overhead
beyond the standard thread introspection options.

The tool runs entirely on the host as a TUI application, displaying the live state of the
application's threads and kernel objects.

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

Then update the workspace, build and flash the application, and run ZView through the integrated
west command:

.. code-block:: sh

   west update
   west build -b <board> -S zview <app>
   west flash
   west zview

.. note::

   The ``zview`` snippet is optional. ZView runs against any build of the application, but without
   the snippet some features, such as stack watermarks, thread names and CPU usage, may be
   unavailable, since they depend on Kconfig options that are disabled by default.

Refer to the `ZView repository <zview_>`_ for the full list of supported options and CLI usage.

Reference
*********

- `ZView repository <zview_>`_

.. _zview: https://github.com/wkhadgar/zview
