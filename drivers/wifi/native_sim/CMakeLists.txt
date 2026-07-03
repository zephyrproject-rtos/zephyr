# SPDX-License-Identifier: Apache-2.0

if(CONFIG_WIFI_NATIVE_SIM)
  # Use 32-bit version of pkg-config
  set(PKG_CONFIG_EXECUTABLE i686-linux-gnu-pkg-config)
  find_package(PkgConfig)
  pkg_check_modules(LIBNL libnl-3.0 libnl-genl-3.0)

  # The driver links the host 32-bit libnl. It is not part of a standard
  # toolchain (e.g. it is absent on CI runners), so when it is missing skip
  # building the driver instead of failing configuration. The interop test that
  # enables this driver is run manually on a host that has the library; a plain
  # CI build then just produces an image without the driver.
  if(NOT LIBNL_FOUND)
    message(STATUS
      "native_sim Wi-Fi: 32-bit libnl-3.0/libnl-genl-3.0 not found, "
      "skipping the driver build. Install the i386 libnl dev packages to "
      "build it.")
    return()
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
  )

  target_link_options(native_simulator INTERFACE
    "${LIBNL_LDFLAGS}"
  )
endif()
