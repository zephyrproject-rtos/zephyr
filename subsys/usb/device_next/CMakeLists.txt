# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library_sources(
	usbd_device.c
	usbd_desc.c
	usbd_ch9.c
	usbd_core.c
	usbd_init.c
	usbd_config.c
	usbd_class.c
	usbd_interface.c
	usbd_endpoint.c
	usbd_msg.c
)

zephyr_library_sources_ifdef(
	CONFIG_USBD_SHELL
	usbd_shell.c
)

zephyr_library_sources_ifdef(
	CONFIG_USBD_LOOPBACK_CLASS
	class/loopback.c
)

zephyr_library_sources_ifdef(
	CONFIG_USBD_CDC_ACM_CLASS
	class/usbd_cdc_acm.c
)

zephyr_include_directories_ifdef(
	CONFIG_USBD_CDC_ECM_CLASS
	${ZEPHYR_BASE}/drivers/ethernet
)
zephyr_library_sources_ifdef(
	CONFIG_USBD_CDC_ECM_CLASS
	class/usbd_cdc_ecm.c
)

zephyr_include_directories_ifdef(
	CONFIG_USBD_CDC_NCM_CLASS
	${ZEPHYR_BASE}/drivers/ethernet
)
zephyr_library_sources_ifdef(
	CONFIG_USBD_CDC_NCM_CLASS
	class/usbd_cdc_ncm.c
)

zephyr_library_sources_ifdef(
	CONFIG_USBD_BT_HCI
	class/bt_hci.c
)

zephyr_library_sources_ifdef(
	CONFIG_USBD_MSC_CLASS
	class/usbd_msc.c
	class/usbd_msc_scsi.c
)

zephyr_linker_sources_ifdef(
	CONFIG_USBD_MSC_CLASS
	SECTIONS class/usbd_msc.ld
)

zephyr_library_sources_ifdef(
	CONFIG_USBD_AUDIO2_CLASS
	class/usbd_uac2.c
)

zephyr_library_sources_ifdef(
	CONFIG_USBD_HID_SUPPORT
	class/usbd_hid.c
	class/usbd_hid_api.c
)

zephyr_linker_sources(DATA_SECTIONS usbd_data.ld)
