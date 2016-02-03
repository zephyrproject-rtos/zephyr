.. _board:

Supported Boards
################

The following boards are currently supported:

.. note::
   Developers can create additional board configurations which allow
   Zephyr-based applications to run on other target systems.

x86 Instruction Set Architectures
=================================

+------------------------+------------------------+
| Board Configuration    | Supported              |
| Reference              | Target Systems         |
+========================+========================+
| :ref:`minnowboard`     | MinnowBoard Max        |
+------------------------+------------------------+
| :ref:`qemu_x86`        | QEMU 2.1               |
+------------------------+------------------------+
| :ref:`quark_d2000_crb` | Quark D2000 Boards     |
+------------------------+------------------------+
| :ref:`quark_se_ctb`    | Quark SE Boards        |
+------------------------+------------------------+
| :ref:`arduino_101`     | Arduino 101 Board      |
+------------------------+------------------------+
| :ref:`galileo`         | Galileo and            |
|                        | Galileo Gen 2          |
+------------------------+------------------------+


ARM (v7-M and v7E-M) Instruction Set Architectures
==================================================

+------------------------+------------------------+
| Board Configuration    | Supported              |
| Reference              | Target Systems         |
+========================+========================+
| :ref:`qemu_cortex_m3`  | QEMU 2.1 + patch       |
+------------------------+------------------------+
| :ref:`arduino_due`     | Arduino Due Board      |
+------------------------+------------------------+
| :ref:`frdm_k64f`       | Freescale Freedom      |
|                        | Development Platform   |
+------------------------+------------------------+

For details on how to flash a Zephyr image, see the
respective board reference documentation.
