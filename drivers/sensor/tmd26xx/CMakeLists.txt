zephyr_library()

zephyr_library_sources(tmd26xx.c)
zephyr_library_sources_ifdef(CONFIG_TMD26XX_TRIGGER tmd26xx_trigger.c)
