zephyr_library()
zephyr_library_include_directories(. ${ZEPHYR_BASE}/subsys/net/ip)

zephyr_library_sources(lldp.c)
