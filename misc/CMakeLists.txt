add_definitions(-D__ZEPHYR_SUPERVISOR__)

zephyr_sources_if_kconfig(printk.c)
zephyr_sources_if_kconfig(reboot.c)
zephyr_sources_if_kconfig(stats.c)
