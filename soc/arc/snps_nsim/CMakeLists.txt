# SPDX-License-Identifier: Apache-2.0
zephyr_compile_options(-mcpu=${GCC_M_CPU} -mno-sdata)
zephyr_compile_options_ifdef(CONFIG_CPU_ARCEM -mmpy-option=wlh1)
zephyr_compile_options_ifdef(CONFIG_CPU_ARCHS -mmpy-option=plus_qmacw)
zephyr_compile_options_ifdef(CONFIG_FLOAT -mfpu=fpuda_all)

zephyr_sources(
  soc.c
  soc_config.c
  )
