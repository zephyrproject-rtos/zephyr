.. _platform:

Platform Development Primer
###########################

Zephyr supports the following hardware platforms configurations:

.. toctree::
   :maxdepth: 1

   basic_cortex_m3.rst
   fsl_frdm_k64f.rst
   basic_atom.rst
   basic_minuteia.rst
   galileo.rst

The following table shows the relationships between platform configuration,
hardware platform, and architecture.

+--------------------------+----------------------------------------+--------------+
| Platform Configuration   | Hardware Platform                      | Architecture |
+==========================+========================================+==============+
| basic_cortex_m3          | QEMU 2.1 + patch                       | ARM v7-M     |
+--------------------------+----------------------------------------+--------------+
| fsl_frdm_k64f            | Freescale Freedom Development Platform | ARM v7E-M    |
+--------------------------+----------------------------------------+--------------+
| basic_atom               | QEMU 2.1                               | X86          |
+--------------------------+----------------------------------------+--------------+
| basic_minuteia           | QEMU 2.1                               | X86          |
+--------------------------+----------------------------------------+--------------+
| galileo                  | Galileo                                | X86          |
+--------------------------+----------------------------------------+--------------+
| galileo                  | Galileo (Gen 2)                        | X86          |
+--------------------------+----------------------------------------+--------------+

.. note::
   Customers can create custom platform configurations for any
   Zephyr-supported hardware platform.
