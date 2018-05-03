zephyr_sources_ifdef(CONFIG_CAN_STM32	stm32_can.c)
zephyr_sources_ifdef(CONFIG_USERSPACE   can_handlers.c)
