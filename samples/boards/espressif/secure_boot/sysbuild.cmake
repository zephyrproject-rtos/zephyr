# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

# Test-only keys. ESP32-C2 and ESP32-C61 are ECDSA-P256. Every other
# in-tree SoC uses RSA-3072. An explicit signing-key sysbuild string wins.
if(SB_CONFIG_SOC_FAMILY_ESPRESSIF_SECURE_BOOT AND
   NOT SB_CONFIG_SOC_FAMILY_ESPRESSIF_SECURE_BOOT_SIGNING_KEY)
  if(BOARD_QUALIFIERS MATCHES "esp32c2" OR BOARD_QUALIFIERS MATCHES "esp32c61")
    set(_espressif_sb_key "ecdsa-p256.pem")
  else()
    set(_espressif_sb_key "rsa-3072.pem")
  endif()
  # CACHE so the image configuration script, which runs in another
  # function scope, can see the path. FORCE keeps a board change in the
  # same build directory on the key that matches that SoC.
  set(ESPRESSIF_SECURE_BOOT_SIGNING_KEY
      "${CMAKE_CURRENT_LIST_DIR}/keys/${_espressif_sb_key}"
      CACHE INTERNAL "Secure Boot v2 test signing key" FORCE)
endif()

# Applied after MCUboot's own board overlay so both images share the
# Secure Boot boot_partition size. A sysbuild/mcuboot/ directory would
# replace MCUboot's prj.conf and SoC fragments, so the overlay is passed
# explicitly.
string(REPLACE "/" "_" _espressif_sb_qualifiers "${BOARD_QUALIFIERS}")
set(mcuboot_EXTRA_DTC_OVERLAY_FILE
    "${CMAKE_CURRENT_LIST_DIR}/mcuboot_boards/${BOARD}_${_espressif_sb_qualifiers}.overlay"
    CACHE INTERNAL "MCUboot Secure Boot partition overlay" FORCE)
set(mcuboot_EXTRA_CONF_FILE
    "${CMAKE_CURRENT_LIST_DIR}/mcuboot.conf"
    CACHE INTERNAL "MCUboot Secure Boot size fragment" FORCE)
