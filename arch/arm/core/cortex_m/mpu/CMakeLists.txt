target_sources_ifdef(CONFIG_ARM_CORE_MPU zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/arm_core_mpu.c)
target_sources_if_kconfig(               zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/arm_mpu.c)
target_sources_if_kconfig(               zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nxp_mpu.c)
