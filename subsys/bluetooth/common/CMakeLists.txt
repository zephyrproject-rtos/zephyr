zephyr_library()

target_sources(subsys__bluetooth__common PRIVATE dummy.c)

target_sources_ifdef(CONFIG_BT_DEBUG subsys__bluetooth__common PRIVATE log.c)
target_sources_ifdef(CONFIG_BT_RPA   subsys__bluetooth__common PRIVATE rpa.c)

target_link_libraries(subsys__bluetooth__common subsys__bluetooth)
