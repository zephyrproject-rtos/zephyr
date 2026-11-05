.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

MC two-board UART tests
=======================

This application runs the UART communication test, the UART message test, and
all eight communication stress cases in one firmware image per board. The stress
cases cover basic transfer, repeated transfer, low and high frame loss, and
timeouts.

In the nRF Connect extension, add two build configurations for
``tests/subsys/mc/board`` on ``nrf54l15dk/nrf54l15/cpuapp``:

* Receiver: ``prj_receiver.conf``
* Sender: ``prj_sender.conf``

To build from a west workspace::

   west build -b nrf54l15dk/nrf54l15/cpuapp tests/subsys/mc/board \
     -d tests/subsys/mc/board/build_receiver --pristine \
     -- -DCONF_FILE=prj_receiver.conf
   west build -b nrf54l15dk/nrf54l15/cpuapp tests/subsys/mc/board \
     -d tests/subsys/mc/board/build_sender --pristine \
     -- -DCONF_FILE=prj_sender.conf

Connect P1.04 (TX) on each board to P1.05 (RX) on the other board, and connect
their grounds. Disable VCOM1 on both boards because it shares these UART20
pins. UART20 carries test data at 1 Mbaud. VCOM0 carries the console at 115200
baud.

Flash the receiver image to the receiver board and the sender image to the
sender board. Open one monitor on each VCOM0 console, then reset the receiver
and sender promptly. Each board reports ten passed cases and
``PROJECT EXECUTION SUCCESSFUL``. Console lines begin with ``[RECEIVER]`` or
``[SENDER]``.
