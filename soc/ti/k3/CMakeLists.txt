# Copyright (c) 2024 Nordic Semiconductor ASA
# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BUILD_WITH_TFA)
  if(CONFIG_TFA_MAKE_BUILD_TYPE_DEBUG)
    set(tfa_build_type "debug")
  else()
    set(tfa_build_type "release")
  endif()
  set(tfa_binary_dir ${CMAKE_BINARY_DIR}/tfa)
  set(tfa_artifact_dir "${tfa_binary_dir}/${CONFIG_TFA_PLATFORM}/${CONFIG_TFA_TARGET_BOARD}/${tfa_build_type}")
endif()

if(CONFIG_SOC_FAMILY_TI_K3_BUILD_TIBOOT3)

  # Load addresses from DTS reserved-memory nodes
  dt_nodelabel(ti_k3_tifs_node NODELABEL "tifs_reserved")
  dt_reg_addr(ti_k3_tifs_load_addr PATH "${ti_k3_tifs_node}")
  dt_nodelabel(ti_k3_boardcfg_node NODELABEL "boardcfg_reserved")
  dt_reg_addr(ti_k3_boardcfg_load_addr PATH "${ti_k3_boardcfg_node}")
  dt_chosen(ti_k3_spl_node PROPERTY "ti,k3-spl-load-region")
  dt_reg_addr(ti_k3_spl_load_addr PATH "${ti_k3_spl_node}")

  # Board config file paths (${BOARD_DIR}/support/)
  set(ti_k3_board_cfg_schema "${BOARD_DIR}/support/schema.yaml")
  set(ti_k3_board_cfg_yaml "${BOARD_DIR}/support/board-cfg.yaml")
  set(ti_k3_sec_cfg_yaml "${BOARD_DIR}/support/sec-cfg.yaml")
  set(ti_k3_pm_cfg_yaml "${BOARD_DIR}/support/pm-cfg.yaml")
  set(ti_k3_rm_cfg_yaml "${BOARD_DIR}/support/rm-cfg.yaml")

  set(ti_k3_tifs_bin "${ZEPHYR_HAL_TI_MODULE_DIR}/zephyr/blobs/ti-sysfw/${CONFIG_SOC_FAMILY_TI_K3_TIFS_BIN}")
  set(ti_k3_tifs_cert_bin "${ZEPHYR_HAL_TI_MODULE_DIR}/zephyr/blobs/ti-sysfw/${CONFIG_SOC_FAMILY_TI_K3_TIFS_CERT_BIN}")

  # Resolve key path relative to Zephyr base
  set(ti_k3_cert_key_path "${ZEPHYR_BASE}/${CONFIG_SOC_FAMILY_TI_K3_CERT_KEY_PATH}")

  # Validate required files
  set(ti_k3_missing "")
  if(NOT EXISTS ${ti_k3_tifs_bin})
    list(APPEND ti_k3_missing "TIFS: ${CONFIG_SOC_FAMILY_TI_K3_TIFS_BIN}")
  endif()
  if(NOT EXISTS ${ti_k3_tifs_cert_bin})
    list(APPEND ti_k3_missing "TIFS cert: ${CONFIG_SOC_FAMILY_TI_K3_TIFS_CERT_BIN}")
  endif()
  if(NOT EXISTS "${ti_k3_cert_key_path}")
    list(APPEND ti_k3_missing "signing key: ${ti_k3_cert_key_path}")
  endif()
  if(NOT EXISTS "${ti_k3_board_cfg_schema}")
    list(APPEND ti_k3_missing "schema: ${ti_k3_board_cfg_schema}")
  endif()
  if(NOT EXISTS "${ti_k3_board_cfg_yaml}")
    list(APPEND ti_k3_missing "board-cfg: ${ti_k3_board_cfg_yaml}")
  endif()
  if(CONFIG_SOC_FAMILY_TI_K3_REQUIRES_SEC_CFG AND NOT EXISTS "${ti_k3_sec_cfg_yaml}")
    list(APPEND ti_k3_missing "sec-cfg: ${ti_k3_sec_cfg_yaml}")
  endif()
  if(CONFIG_SOC_FAMILY_TI_K3_REQUIRES_PM_CFG AND NOT EXISTS "${ti_k3_pm_cfg_yaml}")
    list(APPEND ti_k3_missing "pm-cfg: ${ti_k3_pm_cfg_yaml}")
  endif()
  if(CONFIG_SOC_FAMILY_TI_K3_REQUIRES_RM_CFG AND NOT EXISTS "${ti_k3_rm_cfg_yaml}")
    list(APPEND ti_k3_missing "rm-cfg: ${ti_k3_rm_cfg_yaml}")
  endif()

  if(ti_k3_missing)
    string(REPLACE ";" "\n  - " ti_k3_missing_str "${ti_k3_missing}")
    message(WARNING
      "TI K3: Missing files for tiboot3.bin generation:\n  - ${ti_k3_missing_str}\n"
      "tiboot3.bin will NOT be generated. Run 'west blobs fetch hal_ti' to fetch firmware.")
  else()
    # Output paths
    set(ti_k3_combined_boardcfg "${CMAKE_BINARY_DIR}/zephyr/tiboot3-boardcfg.bin")
    set(ti_k3_tiboot3_bin "${CMAKE_BINARY_DIR}/zephyr/tiboot3.bin")

    # Build combined board config blob command
    set(ti_k3_boardcfg_cmd
      ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/ti_board_config.py
      --schema ${ti_k3_board_cfg_schema}
      --board-cfg ${ti_k3_board_cfg_yaml}
    )
    if(CONFIG_SOC_FAMILY_TI_K3_REQUIRES_SEC_CFG)
      list(APPEND ti_k3_boardcfg_cmd --sec-cfg ${ti_k3_sec_cfg_yaml})
    endif()
    if(CONFIG_SOC_FAMILY_TI_K3_REQUIRES_PM_CFG)
      list(APPEND ti_k3_boardcfg_cmd --pm-cfg ${ti_k3_pm_cfg_yaml})
    endif()
    if(CONFIG_SOC_FAMILY_TI_K3_REQUIRES_RM_CFG)
      list(APPEND ti_k3_boardcfg_cmd --rm-cfg ${ti_k3_rm_cfg_yaml})
    endif()
    list(APPEND ti_k3_boardcfg_cmd --output ${ti_k3_combined_boardcfg} --build-dir ${CMAKE_BINARY_DIR})

    # Generate combined board config blob
    add_custom_command(
      OUTPUT ${ti_k3_combined_boardcfg}
      COMMAND ${ti_k3_boardcfg_cmd}
      DEPENDS ${ti_k3_board_cfg_yaml} ${ti_k3_board_cfg_schema}
      COMMENT "Generating tiboot3.bin board config blob"
    )

    if(CONFIG_BUILD_WITH_TFA)
      set(ti_k3_spl_bin "${tfa_artifact_dir}/${CONFIG_SOC_FAMILY_TI_K3_SPL_BIN}")
    else()
      set(ti_k3_spl_bin "${CMAKE_BINARY_DIR}/zephyr/${CONFIG_SOC_FAMILY_TI_K3_SPL_BIN}")
    endif()

    # Set dependencies (TFA required for AM62L, enforced by Kconfig)
    set(ti_k3_tiboot3_depends ${ti_k3_combined_boardcfg})
    if(CONFIG_BUILD_WITH_TFA)
      list(APPEND ti_k3_tiboot3_depends tfa)
    endif()

    # Generate tiboot3.bin
    add_custom_command(
      OUTPUT ${ti_k3_tiboot3_bin}
      COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/rom_image_gen.py
        --swrv ${CONFIG_SOC_FAMILY_TI_K3_SW_REVISION}
        --spl-bin ${ti_k3_spl_bin}
        --sysfw-bin ${ti_k3_tifs_bin}
        --sysfw-inner-cert ${ti_k3_tifs_cert_bin}
        --boardcfg-blob ${ti_k3_combined_boardcfg}
        --spl-loadaddr ${ti_k3_spl_load_addr}
        --sysfw-loadaddr ${ti_k3_tifs_load_addr}
        --bcfg-loadaddr ${ti_k3_boardcfg_load_addr}
        --key ${ti_k3_cert_key_path}
        --rom-image ${ti_k3_tiboot3_bin}
        --build-dir ${CMAKE_BINARY_DIR}
        --dual-stage-boot
      DEPENDS ${ti_k3_tiboot3_depends}
      COMMENT "Generating tiboot3.bin"
    )

    add_custom_target(tiboot3 ALL DEPENDS ${ti_k3_tiboot3_bin})

  endif()

endif()

if(CONFIG_SOC_FAMILY_TI_K3_BUILD_TISPL)

  # Load addresses from DTS reserved-memory nodes
  dt_nodelabel(ti_k3_tfa_node NODELABEL "tfa_reserved")
  dt_reg_addr(ti_k3_tfa_load_addr PATH "${ti_k3_tfa_node}")
  dt_nodelabel(ti_k3_tifs_node NODELABEL "tifs_reserved")
  dt_reg_addr(ti_k3_tifs_load_addr PATH "${ti_k3_tifs_node}")
  dt_nodelabel(ti_k3_boardcfg_node NODELABEL "boardcfg_reserved")
  dt_reg_addr(ti_k3_boardcfg_load_addr PATH "${ti_k3_boardcfg_node}")
  dt_chosen(ti_k3_sram_path PROPERTY "zephyr,sram")
  dt_reg_addr(ti_k3_app_load_addr PATH "${ti_k3_sram_path}")

  # Board config file paths (${BOARD_DIR}/support/)
  set(ti_k3_board_cfg_schema "${BOARD_DIR}/support/schema.yaml")
  set(ti_k3_board_cfg_yaml "${BOARD_DIR}/support/board-cfg.yaml")
  set(ti_k3_sec_cfg_yaml "${BOARD_DIR}/support/sec-cfg.yaml")
  set(ti_k3_pm_cfg_yaml "${BOARD_DIR}/support/pm-cfg.yaml")
  set(ti_k3_rm_cfg_yaml "${BOARD_DIR}/support/rm-cfg.yaml")
  # TIFS firmware paths (same as tiboot3)
  set(ti_k3_tifs_bin "${ZEPHYR_HAL_TI_MODULE_DIR}/zephyr/blobs/ti-sysfw/${CONFIG_SOC_FAMILY_TI_K3_TIFS_BIN}")
  set(ti_k3_tifs_cert_bin "${ZEPHYR_HAL_TI_MODULE_DIR}/zephyr/blobs/ti-sysfw/${CONFIG_SOC_FAMILY_TI_K3_TIFS_CERT_BIN}")
  # Resolve key path relative to Zephyr base
  set(ti_k3_cert_key_path "${ZEPHYR_BASE}/${CONFIG_SOC_FAMILY_TI_K3_CERT_KEY_PATH}")
  # Validate required files
  set(ti_k3_missing "")
  if(NOT EXISTS ${ti_k3_tifs_bin})
    list(APPEND ti_k3_missing "TIFS: ${CONFIG_SOC_FAMILY_TI_K3_TIFS_BIN}")
  endif()
  if(NOT EXISTS ${ti_k3_tifs_cert_bin})
    list(APPEND ti_k3_missing "TIFS cert: ${CONFIG_SOC_FAMILY_TI_K3_TIFS_CERT_BIN}")
  endif()
  if(NOT EXISTS "${ti_k3_cert_key_path}")
    list(APPEND ti_k3_missing "signing key: ${ti_k3_cert_key_path}")
  endif()
  if(NOT EXISTS "${ti_k3_board_cfg_schema}")
    list(APPEND ti_k3_missing "schema: ${ti_k3_board_cfg_schema}")
  endif()
  if(NOT EXISTS "${ti_k3_board_cfg_yaml}")
    list(APPEND ti_k3_missing "board-cfg: ${ti_k3_board_cfg_yaml}")
  endif()
  if(CONFIG_SOC_FAMILY_TI_K3_REQUIRES_SEC_CFG AND NOT EXISTS "${ti_k3_sec_cfg_yaml}")
    list(APPEND ti_k3_missing "sec-cfg: ${ti_k3_sec_cfg_yaml}")
  endif()
  if(CONFIG_SOC_FAMILY_TI_K3_REQUIRES_PM_CFG AND NOT EXISTS "${ti_k3_pm_cfg_yaml}")
    list(APPEND ti_k3_missing "pm-cfg: ${ti_k3_pm_cfg_yaml}")
  endif()
  if(CONFIG_SOC_FAMILY_TI_K3_REQUIRES_RM_CFG AND NOT EXISTS "${ti_k3_rm_cfg_yaml}")
    list(APPEND ti_k3_missing "rm-cfg: ${ti_k3_rm_cfg_yaml}")
  endif()
  if(ti_k3_missing)
    string(REPLACE ";" "\n  - " ti_k3_missing_str "${ti_k3_missing}")
    message(WARNING
      "TI K3: Missing files for tispl.bin generation:\n  - ${ti_k3_missing_str}\n"
      "tispl.bin will NOT be generated. Run 'west blobs fetch hal_ti' to fetch firmware.")
  else()
    # Output paths
    set(ti_k3_tispl_boardcfg "${CMAKE_BINARY_DIR}/zephyr/tispl-boardcfg.bin")
    set(ti_k3_tispl_bin "${CMAKE_BINARY_DIR}/zephyr/tispl.bin")
    # Build combined board config blob command (same as tiboot3)
    set(ti_k3_tispl_boardcfg_cmd
      ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/ti_board_config.py
      --schema ${ti_k3_board_cfg_schema}
      --board-cfg ${ti_k3_board_cfg_yaml}
    )
    if(CONFIG_SOC_FAMILY_TI_K3_REQUIRES_SEC_CFG)
      list(APPEND ti_k3_tispl_boardcfg_cmd --sec-cfg ${ti_k3_sec_cfg_yaml})
    endif()
    if(CONFIG_SOC_FAMILY_TI_K3_REQUIRES_PM_CFG)
      list(APPEND ti_k3_tispl_boardcfg_cmd --pm-cfg ${ti_k3_pm_cfg_yaml})
    endif()
    if(CONFIG_SOC_FAMILY_TI_K3_REQUIRES_RM_CFG)
      list(APPEND ti_k3_tispl_boardcfg_cmd --rm-cfg ${ti_k3_rm_cfg_yaml})
    endif()
    list(APPEND ti_k3_tispl_boardcfg_cmd --output ${ti_k3_tispl_boardcfg} --build-dir ${CMAKE_BINARY_DIR})
    set(ti_k3_atf_bin "${tfa_artifact_dir}/bl31.bin")
    set(ti_k3_app_bin "${CMAKE_BINARY_DIR}/zephyr/zephyr.bin")
    # Generate tispl board config blob
    add_custom_command(
      OUTPUT ${ti_k3_tispl_boardcfg}
      COMMAND ${ti_k3_tispl_boardcfg_cmd}
      DEPENDS ${ti_k3_board_cfg_yaml} ${ti_k3_board_cfg_schema}
      COMMENT "Generating tispl.bin board config blob"
    )
    # Generate tispl.bin (depends on TFA artifacts and zephyr.bin)
    add_custom_command(
      OUTPUT ${ti_k3_tispl_bin}
      COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/rom_image_gen.py
        --swrv ${CONFIG_SOC_FAMILY_TI_K3_SW_REVISION}
        --spl-bin ${ti_k3_atf_bin}
        --spl-loadaddr ${ti_k3_tfa_load_addr}
        --sysfw-bin ${ti_k3_tifs_bin}
        --sysfw-loadaddr ${ti_k3_tifs_load_addr}
        --sysfw-inner-cert ${ti_k3_tifs_cert_bin}
        --boardcfg-blob ${ti_k3_tispl_boardcfg}
        --bcfg-loadaddr ${ti_k3_boardcfg_load_addr}
        --application-bin ${ti_k3_app_bin}
        --application-loadaddr ${ti_k3_app_load_addr}
        --key ${ti_k3_cert_key_path}
        --rom-image ${ti_k3_tispl_bin}
        --build-dir ${CMAKE_BINARY_DIR}
        --dual-stage-boot
      DEPENDS
        tfa
        ${ti_k3_app_bin}
        ${ti_k3_tispl_boardcfg}
      COMMENT "Generating tispl.bin"
    )
    # Wrapper target to ensure tispl.bin is built
    add_custom_target(tispl ALL DEPENDS ${ti_k3_tispl_bin})
  endif()
endif()

add_subdirectory(./common)
