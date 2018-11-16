zephyr_library()
zephyr_library_include_directories(. ${ZEPHYR_BASE}/subsys/net/ip)

zephyr_library_sources_ifdef(CONFIG_NET_L2_CAN can_l2.c)
