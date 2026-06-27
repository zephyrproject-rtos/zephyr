.. zephyr:code-sample:: coredump-udp-demo-shell
   :name: UDP coredump via shell

   Fault from the Zephyr shell; the coredump is mirrored as UART ``#CD:`` hex and framed UDP.

.. SPDX-License-Identifier: Apache-2.0

Overview
========

Zephyr shell stays running on the serial console. Run **``coredump_crash``**
to fault the board; with **``CONFIG_DEBUG_COREDUMP_BACKEND_LOGGING_UDP``**
the coredump byte stream is mirrored as ``#CD:`` hex on the UART and sent
as framed UDP to **``CONFIG_DEBUG_COREDUMP_LOGGING_UDP_HOST``** (syntax is
whatever ``net_ipaddr_parse()`` accepts, e.g. ``192.0.2.2``, ``192.0.2.2:17777``,
``[2001:db8::1]:17777``; port defaults to **17777** when omitted).

Board overlay **``boards/versalnet_apu.conf``** sets lab-style static IPv4;
adjust for your bench. Networking and UDP coredump options live in
**``prj.conf``** (defaults use ``192.0.2.x`` before the overlay applies).

See :ref:`coredump` and :zephyr_file:`subsys/debug/coredump/coredump_backend_logging_udp.c`.

Build
=====

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/debug/coredump_udp_demos/demo_shell
   :board: versalnet_apu
   :goals: build
   :compact:
