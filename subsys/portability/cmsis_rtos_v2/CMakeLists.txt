# SPDX-License-Identifier: Apache-2.0


add_library(CMSIS INTERFACE)

target_include_directories(CMSIS INTERFACE ${PROJECT_SOURCE_DIR}/include/zephyr/portability)

zephyr_library()
zephyr_library_sources_ifdef(
  CONFIG_CMSIS_RTOS_V2
  thread.c
  kernel.c
  timer.c
  mutex.c
  semaphore.c
  mempool.c
  msgq.c
  event_flags.c
  thread_flags.c
)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
)

zephyr_library_link_libraries(CMSIS)
