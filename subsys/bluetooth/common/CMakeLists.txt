zephyr_library()

zephyr_library_sources(dummy.c)

zephyr_library_sources_ifdef(CONFIG_BT_DEBUG log.c)
zephyr_library_sources_ifdef(CONFIG_BT_RPA   rpa.c)

zephyr_library_link_libraries(subsys__bluetooth)
