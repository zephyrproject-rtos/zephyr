if(CONFIG_NET_ARP OR
    CONFIG_NET_L2_BT OR
    CONFIG_NET_L2_BT_SHELL OR
    CONFIG_NET_L2_DUMMY OR
    CONFIG_NET_L2_ETHERNET)

  zephyr_library()
  zephyr_library_include_directories(. ..)
  zephyr_library_compile_definitions_ifdef(
    CONFIG_NEWLIB_LIBC __LINUX_ERRNO_EXTENSIONS__
    )

  zephyr_library_sources_ifdef(CONFIG_NET_ARP         arp.c)
  zephyr_library_sources_ifdef(CONFIG_NET_L2_BT       bluetooth.c)
  zephyr_library_sources_ifdef(CONFIG_NET_L2_BT_SHELL bluetooth_shell.c)
  zephyr_library_sources_ifdef(CONFIG_NET_L2_DUMMY    dummy.c)
  zephyr_library_sources_ifdef(CONFIG_NET_L2_ETHERNET ethernet.c)

endif()

if(CONFIG_NET_L2_IEEE802154)
  add_subdirectory(ieee802154)
endif()
