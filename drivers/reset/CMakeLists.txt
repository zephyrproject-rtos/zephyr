# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_RESET_GD32 reset_gd32.c)
zephyr_library_sources_ifdef(CONFIG_RESET_RPI_PICO reset_rpi_pico.c)
zephyr_library_sources_ifdef(CONFIG_RESET_AST10X0 reset_ast10x0.c)
zephyr_library_sources_ifdef(CONFIG_RESET_STM32 reset_stm32.c)
