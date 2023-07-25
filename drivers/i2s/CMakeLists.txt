# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/i2s.h)

zephyr_library()

zephyr_library_sources(i2s_common.c)
zephyr_library_sources_ifdef(CONFIG_I2S_SAM_SSC		i2s_sam_ssc.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE		i2s_handlers.c)
zephyr_library_sources_ifdef(CONFIG_I2S_STM32		i2s_ll_stm32.c)
zephyr_library_sources_ifdef(CONFIG_I2S_LITEX		i2s_litex.c)
zephyr_library_sources_ifdef(CONFIG_I2S_MCUX_FLEXCOMM	i2s_mcux_flexcomm.c)
zephyr_library_sources_ifdef(CONFIG_I2S_NRFX		i2s_nrfx.c)
zephyr_library_sources_ifdef(CONFIG_I2S_MCUX_SAI	i2s_mcux_sai.c)
