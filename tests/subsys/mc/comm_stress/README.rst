.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

MC communication stress tests
=============================

These tests exercise Zephyr's ``mc_comm`` implementation with segmented
payloads, asymmetric frame sizes, repeated transfers, packet loss,
duplicates, corruption, shortened frames, disconnections, and timeouts.
The native test uses an in-memory transport and runs without extra source
repositories. Both UART roles use the same implementation as the native test.

Host test
---------

From the root of a west workspace containing this Zephyr checkout, run::

   west twister -p native_sim/native/64 -s mc.comm.stress.native -i

The host test uses bounded iterations and deterministic fault injection.
The eight cases are separate ztests. The lossy cases use 20% and 96% frame
loss. The high-loss case accepts a bounded transmission timeout.

nRF54L15 DK UART test
---------------------

Build the receiver and sender from the Zephyr directory::

   west build -b nrf54l15dk/nrf54l15/cpuapp \
     tests/subsys/mc/comm_stress -d build/mc_receiver --pristine \
     -- -DCONF_FILE=prj_receiver.conf
   west build -b nrf54l15dk/nrf54l15/cpuapp \
     tests/subsys/mc/comm_stress -d build/mc_sender --pristine \
     -- -DCONF_FILE=prj_sender.conf

Flash each image to its corresponding board using ``west flash -d`` and
the appropriate ``--dev-id``. UART20 carries protocol data at 1 Mbaud.
UART30 is the 115200 baud console. Cross the boards' P1.04/TXD1 and
P1.05/RXD1 pins and connect their grounds. The debugger's VCOM1 interface
shares the UART20 pins; disable VCOM1 on both board controllers before
running the wired test. Keep VCOM0 enabled for the console.

Start both console captures before resetting the receiver, then reset the
sender. Both boards run the eight cases in the same order. The UART build
uses the full iteration counts; the high-loss phase can take several
minutes. No external ``micro_core`` sources or libraries are required.
