.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:code-sample:: bmc
   :name: Baseboard Management Controller

   Start the Zephyr BMC subsystem from a minimal application.

Overview
********

This sample shows the intended integration point for the
:file:`subsys/mgmt/bmc` library. The application itself stays small: it enables
the BMC subsystem in Kconfig and calls :c:func:`bmc_init` from ``main()``.

The sample configuration keeps the base bring-up small while still enabling the
web dashboard and websocket terminal support:

- HTTPS is disabled
- JTAG support is disabled
- persistent storage is disabled

The Zephyr shell websocket terminal is enabled on the supported sample
platforms. Host serial capture is enabled on both ``native_sim`` and
``native_sim/native/64`` through sample-specific devicetree overlays and
disabled on ``mps2/an385`` because its CMSDK UART driver does not implement the
async UART API that
:file:`subsys/mgmt/bmc/console_logger.c` requires.

When the sample starts successfully, the BMC HTTP and Redfish services are
started by the subsystem.

Building and Running
********************

Use the standard ``west`` commands to build the sample. For example, for
``native_sim``:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mgmt/bmc
   :host-os: unix
   :board: native_sim
   :goals: build
   :compact:

The sample expects a board configuration that provides IPv4 networking. On
boards without a hardware device identifier, the subsystem uses its built-in
fallback BMC UUID and continues starting normally.
