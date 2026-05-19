.. _external_module_canopennode:

CANopenNode Protocol Stack
##########################

Introduction
************

`CANopenNode`_ is free and open source CANopen protocol stack. Glue code for integrating the stack
with Zephyr is available in a dedicated `CANopenNodeZephyr`_ repository, which includes CANopenNode
as a Git submodule.

Both CANopenNode and CANopenNodeZephyr are licensed under the Apache-2.0 license.

Usage with Zephyr
*****************

To pull in CANopenNodeZephyr as a Zephyr :ref:`module <modules>`, either add it as a West project in
the ``west.yaml`` file or pull it in by adding a submanifest
(e.g. ``zephyr/submanifests/canopennodezephyr.yaml``) file with the following content and run ``west
update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: canopennodezephyr
         url: https://github.com/zephyrproject-rtos/CANopenNodeZephyr.git
         revision: main
         submodules:
           - path: CANopenNode
         path: custom/canopennodezephyr # adjust the path as needed

.. _CANopenNode:
   https://github.com/CANopenNode/CANopenNode

.. _CANopenNodeZephyr:
   https://github.com/zephyrproject-rtos/CANopenNodeZephyr
