# SPDX-License-Identifier: Apache-2.0
zephyr_compile_options(-mcpu=${GCC_M_CPU} -mno-sdata -mmpy-option=6)
zephyr_compile_options_ifdef(CONFIG_FPU -mfpu=fpuda_all)

zephyr_sources(
  soc.c
  sysconf.c
  )

zephyr_sources_ifdef(CONFIG_ARC_IOT_CUSTOM_CPU_IDLE soc_idle.c)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
