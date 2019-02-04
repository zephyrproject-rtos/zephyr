zephyr_sources_ifdef(CONFIG_USERSPACE   hwinfo_handlers.c)
zephyr_sources_ifdef(CONFIG_HWINFO      hwinfo_weak_impl.c)

zephyr_sources_ifdef(CONFIG_HWINFO_STM32      hwinfo_stm32.c)
zephyr_sources_ifdef(CONFIG_HWINFO_NRF        hwinfo_nrf.c)
zephyr_sources_ifdef(CONFIG_HWINFO_MCUX_SIM   hwinfo_mcux_sim.c)
zephyr_sources_ifdef(CONFIG_HWINFO_IMXRT      hwinfo_imxrt.c)

zephyr_sources_ifdef(CONFIG_HWINFO_SHELL hwinfo_shell.c)
