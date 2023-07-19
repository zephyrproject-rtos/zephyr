.. _am62x_m4:

AM62x M4F Core
##############

Overview
********

The Texas Instrument AM62x SoC contains a quad Cortex-A53 cluster and a single
Cortex-M4F core in the MCU domain. This chapter describes all boards with support
for the M4F subsystem.

Currently the following hardware platforms are supported:

.. toctree::
   :maxdepth: 1

   am62x_m4_sk.rst
   am62x_m4_phyboard_lyra.rst

Supported Features
==================

The AM62x M4F platform supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| PINCTRL   | on-chip    | pinctrl                             |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial                              |
+-----------+------------+-------------------------------------+

Other hardware features are not currently supported by the port.
