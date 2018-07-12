zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            arm_core_mpu.c
            IFDEF:${CONFIG_CPU_HAS_ARM_MPU} arm_mpu.c
            IFDEF:${CONFIG_CPU_HAS_NXP_MPU} nxp_mpu.c
)

target_sources(arch_arm PRIVATE ${PRIVATE_SOURCES})
target_include_directories(arch_arm PRIVATE
                           ${CMAKE_CURRENT_LIST_DIR}
                           ${CMAKE_CURRENT_SOURCE_DIR}/arm/include/cortex_m
)
