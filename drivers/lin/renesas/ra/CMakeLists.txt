# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_LIN_RENESAS_RA        lin_renesas_ra.c)
zephyr_library_sources_ifdef(CONFIG_LIN_RENESAS_RA_SCI_B  lin_renesas_ra_sci_b.c)
# zephyr-keep-sorted-stop
