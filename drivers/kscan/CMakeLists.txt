# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_KSCAN_XEC		kscan_mchp_xec.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE	    kscan_handlers.c)
