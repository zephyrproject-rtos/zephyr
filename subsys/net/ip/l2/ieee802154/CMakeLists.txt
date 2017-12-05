zephyr_library()
zephyr_library_include_directories(. ../..)
zephyr_library_compile_definitions_ifdef(
  CONFIG_NEWLIB_LIBC __LINUX_ERRNO_EXTENSIONS__
  )

zephyr_library_sources(
  ieee802154.c
  ieee802154_frame.c
  )

zephyr_library_sources_ifdef(
  CONFIG_NET_L2_IEEE802154_FRAGMENT
  ieee802154_fragment.c
  )

zephyr_library_sources_ifdef(
  CONFIG_NET_L2_IEEE802154_MGMT
  ieee802154_mgmt.c
  )

zephyr_library_sources_ifdef(
  CONFIG_NET_L2_IEEE802154_RADIO_ALOHA
  ieee802154_radio_aloha.c
  )

zephyr_library_sources_ifdef(
  CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA
  ieee802154_radio_csma_ca.c
  )

zephyr_library_sources_ifdef(
  CONFIG_NET_L2_IEEE802154_SECURITY
  ieee802154_security.c
  )

zephyr_library_sources_ifdef(
  CONFIG_NET_L2_IEEE802154_SHELL
  ieee802154_shell.c
  )
