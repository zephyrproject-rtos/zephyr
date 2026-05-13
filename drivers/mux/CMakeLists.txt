# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/mux.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USERSPACE         mux_handlers.c)

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_MUX_GPIO          mux_gpio.c)
zephyr_library_sources_ifdef(CONFIG_MUX_NXP_INPUTMUX  mux_nxp_inputmux.c)
zephyr_library_sources_ifdef(CONFIG_MUX_NXP_TRGMUX    mux_nxp_trgmux.c)
zephyr_library_sources_ifdef(CONFIG_MUX_NXP_XBAR      mux_nxp_xbar.c)
# zephyr-keep-sorted-stop
