# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_USERSPACE   hwinfo_handlers.c)
zephyr_sources_ifdef(CONFIG_HWINFO      hwinfo_weak_impl.c)

zephyr_sources_ifdef(CONFIG_HWINFO_STM32      hwinfo_stm32.c)
zephyr_sources_ifdef(CONFIG_HWINFO_NRF        hwinfo_nrf.c)
zephyr_sources_ifdef(CONFIG_HWINFO_MCUX_RCM   hwinfo_mcux_rcm.c)
zephyr_sources_ifdef(CONFIG_HWINFO_MCUX_SIM   hwinfo_mcux_sim.c)
zephyr_sources_ifdef(CONFIG_HWINFO_MCUX_SYSCON   hwinfo_mcux_syscon.c)
zephyr_sources_ifdef(CONFIG_HWINFO_ESP32      hwinfo_esp32.c)
zephyr_sources_ifdef(CONFIG_HWINFO_IMXRT      hwinfo_imxrt.c)
zephyr_sources_ifdef(CONFIG_HWINFO_SAM        hwinfo_sam.c)
zephyr_sources_ifdef(CONFIG_HWINFO_SAM4L      hwinfo_sam4l.c)
zephyr_sources_ifdef(CONFIG_HWINFO_SAM0       hwinfo_sam0.c)
zephyr_sources_ifdef(CONFIG_HWINFO_LITEX      hwinfo_litex.c)
zephyr_sources_ifdef(CONFIG_HWINFO_PSOC6      hwinfo_psoc6.c)
zephyr_sources_ifdef(CONFIG_HWINFO_GECKO      hwinfo_gecko.c)
zephyr_sources_ifdef(CONFIG_HWINFO_SHELL hwinfo_shell.c)
