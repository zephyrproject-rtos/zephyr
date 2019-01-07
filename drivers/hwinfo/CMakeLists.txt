zephyr_sources_ifdef(CONFIG_USERSPACE   hwinfo_handlers.c)
zephyr_sources_ifdef(CONFIG_HWINFO      hwinfo_weak_impl.c)

zephyr_sources_ifdef(CONFIG_HWINFO_STM32   hwinfo_stm32.c)
