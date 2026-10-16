# SPDX-FileCopyrightText: 2026 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/mux.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USERSPACE         mux_handlers.c)
zephyr_library_sources_ifdef(CONFIG_MUX_TI_AM13E2_XBAR_IN  mux_ti_am13e2_xbar_in.c)
zephyr_library_sources_ifdef(CONFIG_MUX_TI_AM13E2_XBAR_OUT mux_ti_am13e2_xbar_out.c)
