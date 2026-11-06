.. _dtdoctor:

Devicetree diagnostics (``dtdoctor``)
#####################################

``dtdoctor`` is a static analysis tool that helps diagnose Devicetree-related build errors.

It intercepts error messages from the compiler and linker and, when they refer to unresolved
Devicetree device symbols (e.g. ``__device_dts_ord_*``), provides detailed information about what
might be causing the error and how to fix it.

Using dtdoctor
**************

To enable ``dtdoctor``, build with ``-DZEPHYR_SCA_VARIANT=dtdoctor``.

For example:

.. code-block:: shell

   west build -b reel_board samples/basic/blinky -- -DZEPHYR_SCA_VARIANT=dtdoctor

Diagnosed errors
****************

``dtdoctor`` explains the following errors:

- A device whose node is disabled: where its ``status`` property is set, and the nodes, aliases and
  ``/chosen`` properties that refer to it.
- A device whose node is enabled but has no driver: the Kconfig options gating the driver.
- A node identifier that does not resolve to a node, such as ``DT_ALIAS(led0)`` on a board without
  a ``led0`` alias: the missing alias, node label, ``/chosen`` property, compatible instance,
  property or child node, along with the ones that exist.

For example, building :zephyr:code-sample:`blinky` for a board without a ``led0`` alias gives:

.. code-block:: console

   $ west build -b qemu_cortex_m3 samples/basic/blinky -- -DZEPHYR_SCA_VARIANT=dtdoctor
   ...
   main.c:21:40: error: '__device_dts_ord_DT_N_ALIAS_led0_P_gpios_IDX_0_PH_ORD' undeclared here
   ...
   +-----------------------------------------------------------------------------+
   | DT Doctor                                                                   |
   +=============================================================================+
   | DT_ALIAS(led0) refers to the devicetree alias 'led0', which is not defined. |
   |                                                                             |
   | Defined aliases: uart-0, uart-1, uart-2                                     |
   |                                                                             |
   | Try defining the alias in the board devicetree or an overlay, for example:  |
   |                                                                             |
   | / {                                                                         |
   |     aliases {                                                               |
   |         led0 = &<node-label>;                                               |
   |     };                                                                      |
   | };                                                                          |
   +-----------------------------------------------------------------------------+
