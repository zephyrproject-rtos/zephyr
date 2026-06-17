.. zephyr:board:: erbium

Overview
********

The AIFoundry Erbium board configuration is a minimal Zephyr port to a
single core on the 16-core Erbium platform. It exposes one 64-bit RISC-V hart,
an MRAM memory region, a PLIC, and a machine timer.

Hardware
********

The board configuration describes the following hardware:

* 64-bit RISC-V hart with the ``rv64imfc_zicsr_zifencei`` ISA extensions
* 16 MiB MRAM memory region
* Platform-Level Interrupt Controller
* RISC-V machine timer

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

Applications for the ``erbium`` board configuration can be built in the
usual way. For example, with the :zephyr:code-sample:`hello_world` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: erbium
   :goals: build

Flashing and Running
====================

No upstream runner is provided for this board yet.

References
**********

* `Erbium open-source processor <https://github.com/openhwgroup/core-et-erbium>`_
