# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_RENESAS_RA_EXTERNAL_INTERRUPT renesas_ra_external_interrupt.c)
