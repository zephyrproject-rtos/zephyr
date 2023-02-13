# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_PS2_XEC		ps2_mchp_xec.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE           ps2_handlers.c)
zephyr_library_sources_ifdef(CONFIG_PS2_NPCX		ps2_npcx_channel.c)
zephyr_library_sources_ifdef(CONFIG_PS2_NPCX		ps2_npcx_controller.c)
