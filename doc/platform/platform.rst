.. _board:

Supported Boards
###################

The Zephyr Kernel supports the board configurations listed in the
table below. An application can use a board configuration as is,
or it can customize a board configuration by changing its default
kernel configuration settings.

.. note::
   Developers can create new board configurations
   that allow an application to run on other target systems.

+----------------------+-----------------+------------------------+
| Platform             | Instruction Set | Supported              |
| Configuration        | Architecture    | Target Systems         |
+======================+=================+========================+
| minnowboard          | X86             | Minnowboard Max        |
+----------------------+-----------------+------------------------+
| qemu_cortex_m3       | ARM v7-M        | QEMU 2.1 + patch       |
+----------------------+-----------------+------------------------+
| qemu_x86             | X86             | QEMU 2.1               |
+----------------------+-----------------+------------------------+
| quark_d2000_crb      | X86             | Quark D2000 Boards     |
+----------------------+-----------------+------------------------+
| quark_se_ctb         | X86             | Quark SE Boards        |
+----------------------+-----------------+------------------------+
| arduino_101          | X86             | Arduino 101 Board      |
+----------------------+-----------------+------------------------+
| frdm_k64f        | ARM v7E-M       | Freescale Freedom      |
|                      |                 | Development Platform   |
+----------------------+-----------------+------------------------+
| galileo              | X86             | | Galileo              |
|                      |                 | | Galileo (Gen 2)      |
+----------------------+-----------------+------------------------+

The following sections provide details on the respective platforms:

.. toctree::
   :maxdepth: 1

   minnowboard.rst
   qemu_cortex_m3.rst
   basic_minuteia.rst
   frdm_k64f.rst
   galileo.rst

