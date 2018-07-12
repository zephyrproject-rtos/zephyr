zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IF_KCONFIG arc_core_mpu.c
            IF_KCONFIG arc_mpu.c
)

target_sources(arch_arc PRIVATE ${PRIVATE_SOURCES})
