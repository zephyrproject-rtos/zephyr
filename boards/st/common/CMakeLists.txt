# Copyright (c) 2026 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BUILD_WITH_TFM)
  # Flash merged TF-M + Zephyr binary
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)

  if(CONFIG_SOC_FAMILY_STM32_HEX_FILE_NS_APPLI_IN_SECURE_RANGE)
    # When using TF-M, generated script <build>/tfm/api_ns/regression.sh must be
    # run prior programming the device. This script does not set the flash non-secure
    # boundaries hence non-secure image must be programmed using the flash secure address
    # range with TFM_HEX_BASE_ADDRESS_NS.
    dt_chosen(ns_part_path PROPERTY "zephyr,code-partition")
    dt_reg_addr(ns_part_addr PATH "${ns_part_path}")
    dt_chosen(ns_part_flash_path PROPERTY "zephyr,flash")
    dt_reg_addr(ns_part_flash_base PATH "${ns_part_flash_path}")
    math(EXPR TFM_HEX_BASE_ADDRESS_NS
      "${ns_part_addr} - ${ns_part_flash_base} + ${CONFIG_STM32_INT_FLASH_SECURE_BASE_ADDRESS}"
    )
  endif()
endif()
