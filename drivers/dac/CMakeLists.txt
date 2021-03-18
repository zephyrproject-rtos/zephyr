# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_DAC_MCUX_DAC	dac_mcux_dac.c)
zephyr_library_sources_ifdef(CONFIG_DAC_MCUX_DAC32	dac_mcux_dac32.c)
zephyr_library_sources_ifdef(CONFIG_DAC_STM32		dac_stm32.c)
zephyr_library_sources_ifdef(CONFIG_DAC_SAM0		dac_sam0.c)
zephyr_library_sources_ifdef(CONFIG_DAC_DACX0508	dac_dacx0508.c)
zephyr_library_sources_ifdef(CONFIG_DAC_DACX3608	dac_dacx3608.c)
zephyr_library_sources_ifdef(CONFIG_DAC_SHELL		dac_shell.c)
zephyr_library_sources_ifdef(CONFIG_DAC_MCP4725		dac_mcp4725.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE		dac_handlers.c)
