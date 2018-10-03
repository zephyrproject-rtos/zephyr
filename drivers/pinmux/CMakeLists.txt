# Board initialization
zephyr_sources_ifdef(CONFIG_PINMUX_CC2650          pinmux_cc2650.c)
zephyr_sources_ifdef(CONFIG_PINMUX_ESP32           pinmux_esp32.c)
zephyr_sources_ifdef(CONFIG_PINMUX_SIFIVE          pinmux_sifive.c)
zephyr_sources_ifdef(CONFIG_PINMUX_MCUX            pinmux_mcux.c)
zephyr_sources_ifdef(CONFIG_PINMUX_MCUX_LPC        pinmux_mcux_lpc.c)
zephyr_sources_ifdef(CONFIG_PINMUX_QMSI            pinmux_qmsi.c)
zephyr_sources_ifdef(CONFIG_PINMUX_STM32           stm32/pinmux_stm32.c)
zephyr_sources_ifdef(CONFIG_PINMUX_SAM0            pinmux_sam0.c)

# "runtime" pinmux
add_subdirectory_ifdef(CONFIG_PINMUX_DEV dev)
