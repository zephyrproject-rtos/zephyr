zephyr_include_directories(.)
zephyr_sources_ifdef(CONFIG_NET_SOCKETS sockets.c)
