# SPDX-License-Identifier: Apache-2.0

# Use 32-bit version of pkg-config
set(PKG_CONFIG_EXECUTABLE i686-pc-linux-gnu-pkg-config)
find_package(PkgConfig)
pkg_check_modules(LIBNL REQUIRED libnl-3.0 libnl-genl-3.0)

zephyr_library_sources_ifdef(CONFIG_WIFI_NATIVE_SIM
  wifi_drv.c
  mgmt_api.c
  iface_api.c
  wifi_drv_api.c
  )

set(HOSTAP_BASE ${ZEPHYR_HOSTAP_MODULE_DIR})
set(WIFI_NM_WPA_SUPPLICANT_BASE ${HOSTAP_BASE}/wpa_supplicant)
set(HOSTAP_SRC_BASE ${HOSTAP_BASE}/src)

target_sources(native_simulator INTERFACE
  wifi_drv_adapt.c
  wifi_drv_supplicant_adapt.c

  ${HOSTAP_SRC_BASE}/common/wpa_common.c
  ${HOSTAP_SRC_BASE}/common/ieee802_11_common.c
  ${HOSTAP_SRC_BASE}/common/hw_features_common.c
  ${HOSTAP_SRC_BASE}/common/wpa_ctrl.c
  ${HOSTAP_SRC_BASE}/common/cli.c
  ${HOSTAP_SRC_BASE}/common/ctrl_iface_common.c

  ${HOSTAP_SRC_BASE}/drivers/driver_common.c
  ${HOSTAP_SRC_BASE}/drivers/drivers.c
  ${HOSTAP_SRC_BASE}/drivers/netlink.c
  ${HOSTAP_SRC_BASE}/drivers/linux_ioctl.c
  ${HOSTAP_SRC_BASE}/drivers/rfkill.c
  ${HOSTAP_SRC_BASE}/drivers/driver_nl80211.c
  ${HOSTAP_SRC_BASE}/drivers/driver_nl80211_capa.c
  ${HOSTAP_SRC_BASE}/drivers/driver_nl80211_scan.c
  ${HOSTAP_SRC_BASE}/drivers/driver_nl80211_event.c
  ${HOSTAP_SRC_BASE}/drivers/driver_nl80211_monitor.c
  ${HOSTAP_SRC_BASE}/utils/radiotap.c
  ${HOSTAP_SRC_BASE}/utils/base64.c
  ${HOSTAP_SRC_BASE}/utils/common.c
  ${HOSTAP_SRC_BASE}/utils/wpabuf.c
  ${HOSTAP_SRC_BASE}/utils/bitfield.c
  ${HOSTAP_SRC_BASE}/utils/eloop.c
  ${HOSTAP_SRC_BASE}/utils/os_unix.c
  ${HOSTAP_SRC_BASE}/crypto/crypto_linux.c
  ${HOSTAP_SRC_BASE}/crypto/tls_none.c
  ${HOSTAP_SRC_BASE}/l2_packet/l2_packet_linux.c
  ${HOSTAP_SRC_BASE}/utils/wpa_debug.c

  ${WIFI_NM_WPA_SUPPLICANT_BASE}/config.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/notify.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/eap_register.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/op_classes.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/rrm.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/wmm_ac.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/config_none.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/bssid_ignore.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/wpas_glue.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/scan.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/ctrl_iface.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/ctrl_iface_unix.c

  ${WIFI_NM_WPA_SUPPLICANT_BASE}/bss.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/sme.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/wpa_supplicant.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/events.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/robust_av.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/mbo.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/wnm_sta.c
  ${WIFI_NM_WPA_SUPPLICANT_BASE}/wpa_cli_cmds.c
  )

target_compile_options(native_simulator INTERFACE
  "-I${WIFI_NM_WPA_SUPPLICANT_BASE}"
  "-I${HOSTAP_BASE}"
  "-I${HOSTAP_SRC_BASE}"
  "-I${HOSTAP_SRC_BASE}/utils"
  "-I${HOSTAP_SRC_BASE}/drivers"
  "${LIBNL_CFLAGS}"
  "-D_DEFAULT_SOURCE"
  "-DCONFIG_CTRL_IFACE"
  "-DCONFIG_SME"
  "-DCONFIG_MBO"
  "-DCONFIG_WNM"
  "-DCONFIG_WMM_AC"
  "-DCONFIG_NO_CONFIG_WRITE"
  "-DCONFIG_NO_CONFIG_BLOBS"
  "-DCONFIG_NO_RANDOM_POOL"
  "-DCONFIG_NO_WPA"
  "-DCONFIG_NO_PBKDF2"
  "-DCONFIG_SHA256"
  "-DCONFIG_SUITEB192"
  "-DTLS_DEFAULT_CIPHERS=\"DEFAULT:!EXP:!LOW\""
  "-DCONFIG_DRIVER_NL80211"
  "-DCONFIG_DEBUG_FILE"
  "-DCONFIG_RRM"
  "-DCONFIG_ROBUST_AV"
  "-DCONFIG_BSS_MAX_IDLE_TIME"
#  "-DCONFIG_TESTING_OPTIONS"
  )

target_link_options(native_simulator INTERFACE
  "${LIBNL_LDFLAGS}"
  )
