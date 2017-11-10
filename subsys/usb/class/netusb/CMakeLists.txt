zephyr_library()

zephyr_library_include_directories(
  # USB headers
  $ENV{ZEPHYR_BASE}/include/drivers/usb
  $ENV{ZEPHYR_BASE}/include/usb/
  $ENV{ZEPHYR_BASE}/usb/include/
  ..

  # IP headers
  $ENV{ZEPHYR_BASE}/subsys/net/ip
  )

zephyr_library_sources(
  netusb.c
  )

zephyr_library_sources_ifdef(
  CONFIG_USB_DEVICE_NETWORK_ECM
  function_ecm.c
  )
