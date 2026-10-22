# Copyright (c) 2024 GARDENA GmbH
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)

set(CROSSBAR_CONFIG_H ${CMAKE_BINARY_DIR}/zephyr/include/generated/silabs_crossbar_config.h)
add_custom_command(
  OUTPUT ${CROSSBAR_CONFIG_H}
  DEPENDS ${EDT_PICKLE}
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen_crossbar_config.py
                               ${EDT_PICKLE} ${CROSSBAR_CONFIG_H})
add_custom_target(silabs_crossbar_config_h DEPENDS ${CROSSBAR_CONFIG_H})
add_dependencies(zephyr_interface silabs_crossbar_config_h)
