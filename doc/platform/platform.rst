.. _platform:

Platform Development Primer
###########################

Zephyr supports the following hardware platforms configurations:

.. toctree::
   :maxdepth: 1

   fsl_frdm_k64f.rst
   basic_atom.rst
   basic_minuteia.rst
   quark.rst

The following table shows the relationships between platform configuration,
hardware platform, and architecture.

+--------------------------+----------------------------------------+--------------+
| Platform Configuration   | Hardware Platform                      | Architecture |
+==========================+========================================+==============+
| fsl_frdm_k64f            | Freescale Freedom Development Platform | ARM v7E-M    |
+--------------------------+----------------------------------------+--------------+
| basic_atom               | QEMU 2.1                               | X86          |
+--------------------------+----------------------------------------+--------------+
| basic_minuteia           | QEMU 2.1                               | X86          |
+--------------------------+----------------------------------------+--------------+
| quark                    | Galileo                                | X86          |
+--------------------------+----------------------------------------+--------------+
| quark                    | Galileo (Gen 2)                        | X86          |
+--------------------------+----------------------------------------+--------------+

.. note::
   Customers can create custom platform configurations for any
   Zephyr-supported hardware platform.
