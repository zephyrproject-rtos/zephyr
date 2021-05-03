# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MDIO_ATMEL_SAM		mdio_sam.c)
