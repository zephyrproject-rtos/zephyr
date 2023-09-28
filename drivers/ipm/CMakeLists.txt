# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/ipm.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_IPM_MCUX   ipm_mcux.c)
zephyr_library_sources_ifdef(CONFIG_IPM_IMX ipm_imx.c)
zephyr_library_sources_ifdef(CONFIG_IPM_IMX_REV2 ipm_imx.c)
zephyr_library_sources_ifdef(CONFIG_IPM_MHU ipm_mhu.c)
zephyr_library_sources_ifdef(CONFIG_IPM_STM32_IPCC ipm_stm32_ipcc.c)
zephyr_library_sources_ifdef(CONFIG_IPM_NRFX ipm_nrfx_ipc.c)
zephyr_library_sources_ifdef(CONFIG_IPM_STM32_HSEM ipm_stm32_hsem.c)
zephyr_library_sources_ifdef(CONFIG_IPM_CAVS_HOST ipm_cavs_host.c)
zephyr_library_sources_ifdef(CONFIG_IPM_SEDI ipm_sedi.c)
zephyr_library_sources_ifdef(CONFIG_IPM_IVSHMEM ipm_ivshmem.c)
zephyr_library_sources_ifdef(CONFIG_ESP32_SOFT_IPM ipm_esp32.c)
zephyr_library_sources_ifdef(CONFIG_XLNX_IPI ipm_xlnx_ipi.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE   ipm_handlers.c)
