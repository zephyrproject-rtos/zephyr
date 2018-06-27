zephyr_library()
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_compile_definitions_ifdef(
  CONFIG_NEWLIB_LIBC __LINUX_ERRNO_EXTENSIONS__
  )

zephyr_library_sources_ifdef(CONFIG_NET_L2_BT       bluetooth.c)
zephyr_library_sources_ifdef(CONFIG_NET_L2_BT_SHELL bluetooth_shell.c)
