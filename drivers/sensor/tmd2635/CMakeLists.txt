zephyr_library()

zephyr_library_sources(tmd2635.c)
zephyr_library_sources_ifdef(CONFIG_TMD2635_TRIGGER tmd2635_trigger.c)
