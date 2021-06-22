#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(soc.c)
zephyr_library_sources_ifdef(CONFIG_PLATFORM_SPECIFIC_INIT gcc/startup_LPC54114_cm4.S)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

if (CONFIG_SECOND_CORE_MCUX)
  set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
  string(CONFIGURE ${CONFIG_SECOND_IMAGE_MCUX} core_m0_image)

  add_custom_target(second_core_inc_target DEPENDS ${gen_dir}/core-m0.inc)

  generate_inc_file_for_gen_target(${ZEPHYR_CURRENT_LIBRARY}
				   ${core_m0_image}
				   ${gen_dir}/core-m0.inc
				   second_core_inc_target)
endif()
