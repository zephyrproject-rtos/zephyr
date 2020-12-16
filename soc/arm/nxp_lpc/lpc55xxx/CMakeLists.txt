#
# Copyright (c) 2019, NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(soc.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

if (CONFIG_SECOND_CORE_MCUX)
  set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
  string(CONFIGURE ${CONFIG_SECOND_IMAGE_MCUX} second_core_image)

  add_custom_target(second_core_inc_target DEPENDS ${gen_dir}/second-core.inc)

  generate_inc_file_for_gen_target(${ZEPHYR_CURRENT_LIBRARY}
				   ${second_core_image}
				   ${gen_dir}/second-core.inc
				   second_core_inc_target)
endif()
