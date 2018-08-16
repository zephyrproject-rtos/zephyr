zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_CAN_STM32} stm32_can.c
            IFDEF:${CONFIG_USERSPACE} can_handlers.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
