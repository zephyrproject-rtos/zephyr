zephyr_library()

zephyr_library_include_directories(
  # USB headers
  ${ZEPHYR_BASE}/include/drivers/usb
  ${ZEPHYR_BASE}/include/usb/
  ${ZEPHYR_BASE}/subsys/usb/
  ..

  # IP headers
  ${ZEPHYR_BASE}/subsys/net/ip
  )

zephyr_library_sources(
  netusb.c
  )

zephyr_library_sources_ifdef(
  CONFIG_USB_DEVICE_NETWORK_ECM
  function_ecm.c
  )

zephyr_library_sources_ifdef(
  CONFIG_USB_DEVICE_NETWORK_RNDIS
  function_rndis.c
  )

zephyr_library_sources_ifdef(
  CONFIG_USB_DEVICE_NETWORK_EEM
  function_eem.c
  )
