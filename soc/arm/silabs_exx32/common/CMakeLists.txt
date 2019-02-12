zephyr_sources(soc.c soc_gpio.c)

zephyr_sources_ifdef(CONFIG_SYS_POWER_MANAGEMENT soc_power.c)
