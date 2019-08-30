# SPDX-License-Identifier: Apache-2.0


add_library(CMSIS INTERFACE)

target_include_directories(CMSIS INTERFACE ${PROJECT_SOURCE_DIR}/include/cmsis_rtos_v1)

zephyr_library()
zephyr_library_sources_ifdef(
  CONFIG_CMSIS_RTOS_V1
  cmsis_thread.c
  cmsis_wait.c
  cmsis_kernel.c
  cmsis_timer.c
  cmsis_mutex.c
  cmsis_semaphore.c
  cmsis_mempool.c
  cmsis_mailq.c
  cmsis_msgq.c
  cmsis_signal.c
)

zephyr_library_link_libraries(CMSIS)
