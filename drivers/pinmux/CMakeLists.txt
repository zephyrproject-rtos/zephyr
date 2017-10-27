# Board initialization
zephyr_sources_ifdef(CONFIG_PINMUX_CC2650          pinmux_cc2650.c)
zephyr_sources_ifdef(CONFIG_PINMUX_DW              pinmux_dw.c)
zephyr_sources_ifdef(CONFIG_PINMUX_ESP32           pinmux_esp32.c)
zephyr_sources_ifdef(CONFIG_PINMUX_FE310           pinmux_fe310.c)
zephyr_sources_ifdef(CONFIG_PINMUX_MCUX            pinmux_mcux.c)
zephyr_sources_ifdef(CONFIG_PINMUX_QMSI            pinmux_qmsi.c)
zephyr_sources_ifdef(CONFIG_PINMUX_STM32           stm32/pinmux_stm32.c)

# "runtime" pinmux
add_subdirectory_ifdef(CONFIG_PINMUX_DEV dev)

zephyr_sources_ifdef(CONFIG_USERSPACE   pinmux_handlers.c)
