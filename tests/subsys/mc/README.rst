.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

MC tests on two nRF54L15 DKs
============================

Add ``tests/subsys/mc/board`` as an application in nRF Connect for VS Code.
Create one build configuration for each board role using
``nrf54l15dk/nrf54l15/cpuapp`` as the board target:

.. list-table:: Two-board build configurations
   :header-rows: 1

   * - Base configuration file
     - Build directory name
     - Board role
   * - ``prj_receiver.conf``
     - ``build_receiver``
     - Receiver
   * - ``prj_sender.conf``
     - ``build_sender``
     - Sender

Choose the listed file in **Base configuration files**. Leave **Extra Kconfig
fragments** empty; the board overlay is selected automatically. Flash each
image to its matching board. Connect the UART pins and grounds as described in
``board/README.rst``.

Each image runs the communication, message, and eight stress cases in one
boot. Open one VCOM0 console per board at 115200 baud and reset the receiver
and sender promptly. Both boards should report ten passing cases and
``PROJECT EXECUTION SUCCESSFUL``. The separate ``uart`` and ``comm_stress``
applications remain available for focused test runs.
