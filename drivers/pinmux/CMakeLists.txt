# Board initialization

# "runtime" pinmux
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_PINMUX_CC2650}      pinmux_cc2650.c
            IFDEF:${CONFIG_PINMUX_ESP32}       pinmux_esp32.c
            IFDEF:${CONFIG_PINMUX_FE310}       pinmux_fe310.c
            IFDEF:${CONFIG_PINMUX_MCUX}        pinmux_mcux.c
            IFDEF:${CONFIG_PINMUX_MCUX_LPC}    pinmux_mcux_lpc.c
            IFDEF:${CONFIG_PINMUX_QMSI}        pinmux_qmsi.c
            IFDEF:${CONFIG_PINMUX_STM32}       stm32/pinmux_stm32.c
            IFDEF:${CONFIG_PINMUX_SAM0}        pinmux_sam0.c
            IFDEF:${CONFIG_PINMUX_INTEL_S1000} pinmux_intel_s1000.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

include_relative_ifdef(CONFIG_PINMUX_DEV dev/CMakeLists.txt)
