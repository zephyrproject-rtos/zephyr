# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# Common USB-C Stack files
zephyr_library_sources_ifdef(
	CONFIG_USBC_STACK
	usbc_timer.c
	usbc_stack.c
	usbc_tc_common.c
	usbc_pe_common.c
	usbc_prl.c
)

# Sink USB-C Stack files
zephyr_library_sources_ifdef(
	CONFIG_USBC_CSM_SINK_ONLY
	usbc_tc_snk_states.c
	usbc_pe_snk_states.c
)
