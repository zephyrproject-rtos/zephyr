zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_CMSIS_RTOS_V1} cmsis_thread.c
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

add_library(cmsis STATIC ${PRIVATE_SOURCES})
target_include_directories(cmsis PUBLIC ${PROJECT_SOURCE_DIR}/include/cmsis_rtos_v1)
target_link_libraries(cmsis PRIVATE zephyr_interface)
target_link_libraries(zephyr_app_linking INTERFACE cmsis)
