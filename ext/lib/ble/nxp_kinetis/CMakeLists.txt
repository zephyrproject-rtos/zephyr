# Copyright (c) 2018 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

# N.B.: Temporary. See the README but currently need to manually copy
# the BLE Link Layer controller library from the NXP KW41Z SDK. NXP is
# working on a more seamless solution using an NXP managed git
# repository.

if(CONFIG_BT_LL_KW41Z)
  add_library(kw41z_ble STATIC IMPORTED GLOBAL)
  set_target_properties(kw41z_ble PROPERTIES IMPORTED_LOCATION
    ${ZEPHYR_BASE}/ext/lib/ble/nxp_kinetis/lib_ble_kw41z_controller.a
    )
  zephyr_append_cmake_library(kw41z_ble)
endif()
