.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

MC UART communication test
==========================

This two-board sample runs the ``micro_core`` UDP communication test cases
over UART20 instead of UDP sockets. It builds the communication implementation
from a separate ``micro_core`` checkout, selected by ``MICRO_CORE_ROOT``.
It does not exercise Zephyr's ``CONFIG_MC_COMM`` or ``CONFIG_MC_MSG`` APIs.

Both roles print the original test case names and elapsed times. Each board
also prints its own role-labelled ASCII progress bar on every transfer iteration,
completed and failed frame counters, loss
percentages, and throughput. Transport fault injection includes duplicated
frames, packet drops, bit flips, malformed headers, shortened logical frames,
and periodic disconnections. The 20% and 96% lossy phases use the same rates
as the UDP test; actual measured percentages vary.

See ``setup.md`` for the source checkout requirement, board mapping, wiring,
build and flash commands, console capture, and expected output.
