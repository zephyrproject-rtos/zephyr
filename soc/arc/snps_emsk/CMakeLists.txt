# SPDX-License-Identifier: Apache-2.0
zephyr_compile_options(-mcpu=${GCC_M_CPU} -mno-sdata -mmpy-option=6)

if(CONFIG_SOC_EMSK_EM9D)
zephyr_compile_options_ifdef(CONFIG_FPU -mfpu=fpus_all)
elseif(CONFIG_SOC_EMSK_EM11D)
zephyr_compile_options_ifdef(CONFIG_FPU -mfpu=fpuda_all)
endif()

zephyr_sources(
  soc.c
  soc_config.c
  )
