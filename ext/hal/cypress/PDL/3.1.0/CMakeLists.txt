#
# Copyright (c) 2018, Cypress
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_compile_definitions(${CONFIG_SOC_PART_NUMBER})

# Workaround to fix absence of four defines in PDL
# The values don't matter
zephyr_compile_definitions(
__copy_table_start__=0
)
zephyr_compile_definitions(
__copy_table_end__=0
)
zephyr_compile_definitions(
__bss_start__=0
)
zephyr_compile_definitions(
__bss_end__=0
)

zephyr_include_directories(drivers/include)
zephyr_sources(drivers/source/cy_sysclk.c)
zephyr_include_directories(devices/psoc6/include)
zephyr_include_directories(devices/psoc6/include/ip)

zephyr_sources(devices/psoc6/startup/system_psoc6_cm0plus.c)
zephyr_sources(devices/psoc6/startup/gcc/startup_psoc62_cm0plus.S)
zephyr_sources(drivers/source/cy_device.c)
zephyr_sources(drivers/source/cy_syslib.c)
zephyr_sources(drivers/source/cy_sysint.c)
zephyr_sources(drivers/source/gcc/cy_syslib_gcc.S)
zephyr_sources(drivers/source/cy_gpio.c)
zephyr_sources(drivers/source/cy_prot.c)
zephyr_sources(drivers/source/cy_syspm.c)
zephyr_sources(drivers/source/cy_sysclk.c)
zephyr_sources(drivers/source/cy_flash.c)
zephyr_sources(drivers/source/cy_ipc_drv.c)
zephyr_sources(drivers/source/cy_ipc_config.c)
zephyr_sources(drivers/source/cy_ipc_sema.c)
zephyr_sources(drivers/source/cy_ipc_pipe.c)
zephyr_sources(drivers/source/cy_scb_uart.c)
zephyr_sources(drivers/source/cy_scb_common.c)
