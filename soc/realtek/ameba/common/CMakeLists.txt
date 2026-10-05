# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(ameba_irq.c)
if(CONFIG_CPU_CORTEX_M)
# reset_ameba_shell.c registers a `reboot uartburn` SHELL command.
zephyr_sources(reset_ameba_shell.c)
endif()
