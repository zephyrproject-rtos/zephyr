
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
)

zephyr_library_link_libraries(CMSIS)
target_link_libraries(CMSIS INTERFACE zephyr_interface)
