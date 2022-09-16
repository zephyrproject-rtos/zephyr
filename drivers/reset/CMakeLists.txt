# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_RESET_RPI_PICO reset_rpi_pico.c)
