.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

Micro Core UART sample
######################

This two-board sample exchanges a payload and an empty payload through the in-tree
:c:func:`mc_comm_send` and :c:func:`mc_comm_recv` API. It demonstrates caller-owned, aligned
instance storage and a complete-frame UART adapter. The sender and receiver need separate
nRF54L15 DK boards.

Connect P1.04 (TX) of each board to P1.05 (RX) of the other board, and connect their grounds.
UART20 carries the protocol; UART30 remains the console at 115200 baud. Build the roles from
the Zephyr directory::

   west build -b nrf54l15dk/nrf54l15/cpuapp samples/subsys/micro_core/uart \
     -d build/mc-sample-sender --pristine -- -DCONF_FILE=prj_sender.conf
   west build -b nrf54l15dk/nrf54l15/cpuapp samples/subsys/micro_core/uart \
     -d build/mc-sample-receiver --pristine -- -DCONF_FILE=prj_receiver.conf

Flash each image to its matching board. Start the receiver before the sender. A successful
exchange prints ``Micro Core UART: PASS`` on both consoles. This sample has build-only Twister
metadata because it requires two wired boards.
