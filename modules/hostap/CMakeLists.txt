#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_WIFI_NM_WPA_SUPP)

zephyr_library()

set(HOSTAP_BASE ${ZEPHYR_HOSTAP_MODULE_DIR})
set(WIFI_NM_WPA_SUPPLICANT_BASE ${HOSTAP_BASE}/wpa_supplicant)
set(COMMON_SRC_BASE ${HOSTAP_BASE}/src)

set(CMAKE_EXE_LINKER_FLAGS "--specs=nosys.specs -lnosys")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMISSING_SYSCALL_NAMES")

zephyr_compile_definitions(
	CONFIG_ZEPHYR
)

zephyr_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
	${HOSTAP_BASE}/
	${WIFI_NM_WPA_SUPPLICANT_BASE}/
	${COMMON_SRC_BASE}/
)

zephyr_library_compile_definitions(
	TLS_DEFAULT_CIPHERS=\""DEFAULT:!EXP:!LOW"\"
	CONFIG_SME
	CONFIG_NO_CONFIG_WRITE
	CONFIG_NO_CONFIG_BLOBS
	CONFIG_CTRL_IFACE
	CONFIG_CTRL_IFACE_ZEPHYR
	CONFIG_NO_RANDOM_POOL
	CONFIG_NO_WPA
	CONFIG_CRYPTO_INTERNAL
  CONFIG_NO_PBKDF2
	)

zephyr_library_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${HOSTAP_BASE}/
	${COMMON_SRC_BASE}/utils
	${COMMON_SRC_BASE}/drivers
	${HOSTAP_BASE}/src
	${ZEPHYR_BASE}/include
	${ZEPHYR_BASE}/include/net
)

zephyr_library_sources(
	${COMMON_SRC_BASE}/common/wpa_common.c
	${COMMON_SRC_BASE}/common/ieee802_11_common.c
	${COMMON_SRC_BASE}/common/hw_features_common.c
	${COMMON_SRC_BASE}/common/wpa_ctrl.c
	${COMMON_SRC_BASE}/common/cli.c

	${COMMON_SRC_BASE}/drivers/driver_common.c
	${COMMON_SRC_BASE}/drivers/drivers.c
	${COMMON_SRC_BASE}/l2_packet/l2_packet_zephyr.c
	${COMMON_SRC_BASE}/drivers/driver_zephyr.c
	${COMMON_SRC_BASE}/utils/base64.c
	${COMMON_SRC_BASE}/utils/common.c
	${COMMON_SRC_BASE}/utils/wpabuf.c
	${COMMON_SRC_BASE}/utils/bitfield.c
	${COMMON_SRC_BASE}/utils/eloop.c
	${COMMON_SRC_BASE}/utils/os_zephyr.c
	${COMMON_SRC_BASE}/utils/radiotap.c
	${COMMON_SRC_BASE}/crypto/crypto_none.c
  ${COMMON_SRC_BASE}/crypto/tls_none.c

	${WIFI_NM_WPA_SUPPLICANT_BASE}/config.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/notify.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/bss.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/eap_register.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/op_classes.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/rrm.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/wmm_ac.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/config_none.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/sme.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/wpa_supplicant.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/events.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/bssid_ignore.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/wpas_glue.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/scan.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/robust_av.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/ctrl_iface.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/ctrl_iface_zephyr.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/wpa_cli_cmds.c
	${WIFI_NM_WPA_SUPPLICANT_BASE}/wpa_cli_zephyr.c
	# Zephyr specific
	supp_main.c
	src/utils/wpa_debug.c
	supp_api.c
  supp_events.c
)

endif()
