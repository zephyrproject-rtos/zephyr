# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USBC_STACK usbc_timer.c usbc_stack.c usbc_tc.c usbc_prl.c usbc_pe.c)
