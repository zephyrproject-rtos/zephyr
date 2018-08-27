zephyr_library()

zephyr_library_sources_codegen_ifdef(CONFIG_PINCTRL_STM32	pinctrl_stm32.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE  	pinctrl_handlers.c)
