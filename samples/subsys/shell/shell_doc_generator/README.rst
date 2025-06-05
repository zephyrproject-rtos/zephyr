Shell Documentation Generator Sample
====================================

Overview
--------
This sample demonstrates how to use Zephyr's shell API and introspection to generate and print hierarchical documentation of all available shell commands and subcommands at runtime.

Building and Running
--------------------

.. code-block:: console

   west build -b qemu_x86 samples/subsys/shell/shell_doc_generator -p -t run

You should see a hierarchical list of all shell commands and their documentation printed to the console.