# SPDX-License-Identifier: Apache-2.0

ExternalZephyrProject_Add(
  APPLICATION remote
  SOURCE_DIR ${APP_DIR}/remote
  BOARD ${SB_CONFIG_REMOTE_BOARD}
)

add_dependencies(${DEFAULT_IMAGE} remote)

get_target_property(primary_build_dir ${DEFAULT_IMAGE} _EP_BINARY_DIR)
get_target_property(remote_build_dir remote _EP_BINARY_DIR)

set(merged_hex ${CMAKE_BINARY_DIR}/rp2350_heterogeneous.hex)
set(merged_uf2 ${CMAKE_BINARY_DIR}/rp2350_heterogeneous.uf2)

add_custom_command(
  OUTPUT ${merged_hex} ${merged_uf2}
  COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
          -o ${merged_hex} --overlap error
          ${primary_build_dir}/zephyr/zephyr.hex
          ${remote_build_dir}/zephyr/zephyr.hex
  COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/uf2conv.py
          -c -f 0xe48bff57 -o ${merged_uf2} ${merged_hex}
  DEPENDS ${DEFAULT_IMAGE} remote
          ${primary_build_dir}/zephyr/zephyr.hex
          ${remote_build_dir}/zephyr/zephyr.hex
  VERBATIM
)

add_custom_target(rp2350_heterogeneous_uf2 ALL DEPENDS ${merged_hex} ${merged_uf2})
