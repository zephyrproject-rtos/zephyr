# SPDX-License-Identifier: Apache-2.0

add_definitions(-D__ZEPHYR_SUPERVISOR__)

include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

add_subdirectory(common)
add_subdirectory(${ARCH_DIR}/${ARCH} arch/${ARCH})
