# SPDX-License-Identifier: Apache-2.0

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNL libnl-3.0 libnl-genl-3.0)

if(NOT LIBNL_FOUND)
  message(FATAL_ERROR
    "native_sim Wi-Fi: libnl-3.0/libnl-genl-3.0 not found. If building for the "
    "32-bit native_sim, install the i386 libnl dev packages and set "
    "PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig to build the driver.")
endif()

zephyr_library_sources(wifi_drv.c
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

  # Driver-only ("wpa_priv" model): we compile the nl80211 driver and its
  # support code, but NOT the full host wpa_supplicant. Our adapter provides
  # wpa_supplicant_event() and forwards events to the Zephyr supplicant.
  ${HOSTAP_SRC_BASE}/common/wpa_common.c
  ${HOSTAP_SRC_BASE}/common/ieee802_11_common.c
  ${HOSTAP_SRC_BASE}/common/hw_features_common.c

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
)

list(TRANSFORM LIBNL_INCLUDE_DIRS PREPEND "-I" OUTPUT_VARIABLE LIBNL_INCLUDE_DIRS_OPTION)
list(TRANSFORM LIBNL_LIBRARY_DIRS PREPEND "-L" OUTPUT_VARIABLE LIBNL_LIBRARY_DIRS_OPTION)
list(TRANSFORM LIBNL_LIBRARIES PREPEND "-l" OUTPUT_VARIABLE LIBNL_LIBRARIES_OPTION)

target_compile_options(native_simulator INTERFACE
  "-I${WIFI_NM_WPA_SUPPLICANT_BASE}"
  "-I${HOSTAP_BASE}"
  "-I${HOSTAP_SRC_BASE}"
  "-I${HOSTAP_SRC_BASE}/utils"
  "-I${HOSTAP_SRC_BASE}/drivers"
  ${LIBNL_INCLUDE_DIRS_OPTION}
  ${LIBNL_CFLAGS_OTHER}
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
)

target_link_options(native_simulator INTERFACE
  ${LIBNL_LIBRARY_DIRS_OPTION}
  ${LIBNL_LIBRARIES_OPTION}
)
