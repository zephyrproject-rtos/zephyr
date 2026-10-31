.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

MC communication stress tests
=============================

These tests check long messages, lost or damaged frames, and timeouts. The
simulator and QEMU tests use an in-memory link. No external ``micro_core``
library is needed.

Simulator and x86 QEMU
---------------------

Run one command from the Zephyr directory in a west workspace::

   west twister -p native_sim/native/64 -s mc.comm.stress.native -i
   west twister -p qemu_x86/atom -s mc.comm.stress.qemu_x86 -i
   west twister -p qemu_x86_64/atom -s mc.comm.stress.qemu_x86_64 -i

QEMU needs an installed Zephyr toolchain and QEMU. Each command runs eight tests.

nRF54L15 DK UART test
---------------------

Build one image for each board from the Zephyr directory::

   west build -b nrf54l15dk/nrf54l15/cpuapp \
     tests/subsys/mc/comm_stress -d build/mc_receiver --pristine \
     -- -DCONF_FILE=prj_receiver.conf
   west build -b nrf54l15dk/nrf54l15/cpuapp \
     tests/subsys/mc/comm_stress -d build/mc_sender --pristine \
     -- -DCONF_FILE=prj_sender.conf

Connect P1.04 (TX) on each board to P1.05 (RX) on the other board. Connect
their grounds. UART20 uses 1 Mbaud. Disable VCOM1 on both boards because it
shares the UART20 pins. Keep VCOM0 for the console.

Flash the images. Replace the device IDs with your board IDs::

   west flash -d build/mc_receiver --dev-id <receiver-id>
   west flash -d build/mc_sender --dev-id <sender-id>

Open both VCOM0 consoles at 115200 baud. Reset the receiver, then the sender.
Both consoles should print ``PROJECT EXECUTION SUCCESSFUL``. The test with
high frame loss may take several minutes.
