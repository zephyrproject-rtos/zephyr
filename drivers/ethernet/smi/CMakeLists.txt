# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SMI_SHELL smi_shell.c)
zephyr_library_sources_ifdef(CONFIG_SMI_GPIO smi_gpio.c)
