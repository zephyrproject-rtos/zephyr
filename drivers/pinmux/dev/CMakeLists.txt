zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_PINMUX_DEV_ARM_V2M_BEETLE} pinmux_dev_arm_beetle.c
            IFDEF:${CONFIG_PINMUX_DEV_ATMEL_SAM3X}    pinmux_dev_atmel_sam3x.c
            IFDEF:${CONFIG_PINMUX_DEV_STM32}          pinmux_dev_stm32.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
