zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            netusb.c
            IFDEF:${CONFIG_USB_DEVICE_NETWORK_ECM} function_ecm.c
            IFDEF:${CONFIG_USB_DEVICE_NETWORK_RNDIS} function_rndis.c
            IFDEF:${CONFIG_USB_DEVICE_NETWORK_EEM} function_eem.c
)

target_sources(subsys_usb PRIVATE ${PRIVATE_SOURCES})
target_include_directories(subsys_usb PRIVATE
  # USB headers
  ${ZEPHYR_BASE}/include/drivers/usb
  ${ZEPHYR_BASE}/include/usb/
  ..

  # IP headers
  ${ZEPHYR_BASE}/subsys/net/ip
)
