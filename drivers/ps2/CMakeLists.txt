# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/ps2.h)

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_PS2_IT51XXX ps2_it51xxx.c)
zephyr_library_sources_ifdef(CONFIG_PS2_NPCX ps2_npcx_channel.c)
zephyr_library_sources_ifdef(CONFIG_PS2_NPCX ps2_npcx_controller.c)
zephyr_library_sources_ifdef(CONFIG_PS2_XEC ps2_mchp_xec.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE ps2_handlers.c)
# zephyr-keep-sorted-stop
