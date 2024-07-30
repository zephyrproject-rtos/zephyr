# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/retained_mem.h)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_USERSPACE retained_mem_handlers.c)
zephyr_library_sources_ifdef(CONFIG_RETAINED_MEM_NRF_GPREGRET retained_mem_nrf_gpregret.c)
zephyr_library_sources_ifdef(CONFIG_RETAINED_MEM_ZEPHYR_RAM   retained_mem_zephyr_ram.c)
zephyr_library_sources_ifdef(CONFIG_RETAINED_MEM_ZEPHYR_REG   retained_mem_zephyr_reg.c)
