# SPDX-License-Identifier: Apache-2.0


add_library(CMSIS INTERFACE)

target_include_directories(CMSIS INTERFACE ${PROJECT_SOURCE_DIR}/include/cmsis_rtos_v2)

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

zephyr_library_link_libraries(CMSIS)
