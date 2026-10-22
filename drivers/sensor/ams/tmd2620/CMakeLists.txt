zephyr_library()

zephyr_library_sources(tmd2620.c)
zephyr_library_sources_ifdef(CONFIG_TMD2620_TRIGGER tmd2620_trigger.c)
