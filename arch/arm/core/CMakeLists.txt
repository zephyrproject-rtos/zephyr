target_sources(zephyr PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/exc_exit.S
  ${CMAKE_CURRENT_SOURCE_DIR}/irq_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/swap.S
  ${CMAKE_CURRENT_SOURCE_DIR}/fault.c
  ${CMAKE_CURRENT_SOURCE_DIR}/irq_manage.c
  ${CMAKE_CURRENT_SOURCE_DIR}/thread.c
  ${CMAKE_CURRENT_SOURCE_DIR}/cpu_idle.S
  ${CMAKE_CURRENT_SOURCE_DIR}/fault_s.S
  ${CMAKE_CURRENT_SOURCE_DIR}/fatal.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sys_fatal_error_handler.c
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_abort.c
)

target_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/isr_wrapper.S)
target_sources_ifdef(CONFIG_CPLUSPLUS zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/__aeabi_atexit.c)
target_sources_ifdef(CONFIG_IRQ_OFFLOAD zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/irq_offload.c)

add_subdirectory(offsets)

add_subdirectory_ifdef(CONFIG_CPU_CORTEX_M cortex_m)
add_subdirectory_ifdef(CONFIG_CPU_HAS_MPU  cortex_m/mpu)
