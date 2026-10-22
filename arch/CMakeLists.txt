# SPDX-License-Identifier: Apache-2.0

# FIXME: SHADOW_VARS: Remove this once we have enabled -Wshadow globally.
add_compile_options($<TARGET_PROPERTY:compiler,warning_shadow_variables>)

add_definitions(-D__ZEPHYR_SUPERVISOR__)

include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
  )

add_subdirectory(common)
add_subdirectory(${ARCH_DIR}/${ARCH} arch/${ARCH})
