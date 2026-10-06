# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  precision_clock.c
  precision_pi.c
  precision_time.c
)

# Adapter that bridges the PTP clock driver API onto the protocol-neutral
# precision timing clock abstraction.
zephyr_library_sources_ifdef(CONFIG_PTP_CLOCK precision_clock_ptp.c)

zephyr_library_sources_ifdef(CONFIG_PRECISION_PPS_OUTPUT
  precision_pps_output.c
)

zephyr_library_sources_ifdef(CONFIG_PRECISION_TIMING_SHELL
  precision_timing_shell.c
)
