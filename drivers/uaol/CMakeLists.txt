# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_UAOL_INTEL_ADSP uaol_intel_adsp.c)

if(CONFIG_UAOL_USERSPACE)
  zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/uaol.h)
  zephyr_library_sources(uaol_handlers.c)
endif()
