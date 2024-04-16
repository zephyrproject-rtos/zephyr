# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/charger.h)

zephyr_library_sources_ifdef(CONFIG_SBS_CHARGER sbs_charger.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE charger_handlers.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_SBS_CHARGER emul_sbs_charger.c)
