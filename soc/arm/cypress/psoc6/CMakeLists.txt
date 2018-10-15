#
# Copyright (c) 2018, Cypress
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(.)
zephyr_sources(
	soc.c
)

if (CONFIG_SLAVE_CORE_PSOC6)
  set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
  string(CONFIGURE ${CONFIG_SLAVE_IMAGE_PSOC6} core_m4_image)

  add_custom_target(core_m4_inc_target DEPENDS ${gen_dir}/core-m4.inc)

  generate_inc_file_for_gen_target(${ZEPHYR_CURRENT_LIBRARY}
				   ${core_m4_image}
				   ${gen_dir}/core-m4.inc
				   core_m4_inc_target)
endif()
