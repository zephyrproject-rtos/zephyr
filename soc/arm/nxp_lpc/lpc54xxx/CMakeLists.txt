#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(soc.c)

if (CONFIG_SLAVE_CORE_MCUX)
  set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
  string(CONFIGURE ${CONFIG_SLAVE_IMAGE_MCUX} core_m0_image)

  add_custom_target(core_m0_inc_target DEPENDS ${gen_dir}/core-m0.inc)

  generate_inc_file_for_gen_target(${ZEPHYR_CURRENT_LIBRARY}
				   ${core_m0_image}
				   ${gen_dir}/core-m0.inc
				   core_m0_inc_target)
endif()
