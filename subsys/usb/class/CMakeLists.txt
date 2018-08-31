zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_USB_CDC_ACM} cdc_acm.c
            IFDEF:${CONFIG_USB_MASS_STORAGE} mass_storage.c
            IFDEF:${CONFIG_USB_DEVICE_BLUETOOTH} bluetooth.c
            IFDEF:${CONFIG_USB_DFU_CLASS} usb_dfu.c
            IFDEF:${CONFIG_USB_DEVICE_LOOPBACK} loopback.c
)

target_sources(subsys_usb PRIVATE ${PRIVATE_SOURCES})
target_include_directories(subsys_usb PRIVATE ${ZEPHYR_BASE}/subsys/usb)

include_relative_ifdef(CONFIG_USB_DEVICE_NETWORK netusb/CMakeLists.txt)
include_relative_ifdef(CONFIG_USB_DEVICE_HID hid/CMakeLists.txt)
