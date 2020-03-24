# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ESPI_XEC		espi_mchp_xec.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE           espi_handlers.c)
