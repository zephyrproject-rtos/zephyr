# SPDX-License_Identifier: Apache-2.0

zephyr_library_sources(fido2_transport.c)
zephyr_library_sources_ifdef(CONFIG_FIDO2_TRANSPORT_USB_HID fido2_transport_usb_hid.c)
