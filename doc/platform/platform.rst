.. _platform:

Supported Platforms
###################

The Zephyr kernel supports the platforms configurations listed in the
table below. An application can use a platform configuration as is,
or it can customize the platform configuration by changing its default
kernel configuration settings.

.. note::
   Developers are also free to create new platform configurations
   to allow an application to run on a target systems that is not listed.

+----------------------+-----------------+------------------------+
| Platform             | Instruction Set | Supported              |
| Configuration        | Architecture    | Target Systems         |
+======================+=================+========================+
| basic_atom           | X86             | QEMU 2.1               |
+----------------------+-----------------+------------------------+
| basic_cortex_m3      | ARM v7-M        | QEMU 2.1 + patch       |
+----------------------+-----------------+------------------------+
| basic_minuteia       | X86             | QEMU 2.1               |
+----------------------+-----------------+------------------------+
| fsl_frdm_k64f        | ARM v7E-M       | Freescale Freedom      |
|                      |                 | Development Platform   |
+----------------------+-----------------+------------------------+
| galileo              | X86             | | Galileo              |
|                      |                 | | Galileo (Gen 2)      |
+----------------------+-----------------+------------------------+

The following sections provide details on the associated platform configuration:

.. toctree::
   :maxdepth: 1

   basic_atom.rst
   basic_cortex_m3.rst
   basic_minuteia.rst
   fsl_frdm_k64f.rst
   galileo.rst

