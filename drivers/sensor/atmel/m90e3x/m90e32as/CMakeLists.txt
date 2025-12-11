# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(m90e32as.c)

zephyr_library_sources_ifdef(CONFIG_SPI ../m90e3x_spi.c)
zephyr_library_sources_ifdef(CONFIG_PM_DEVICE ../m90e3x_pm.c)
zephyr_library_sources_ifdef(CONFIG_M90EXX_SHELL ../m90e3x_shell.c)
