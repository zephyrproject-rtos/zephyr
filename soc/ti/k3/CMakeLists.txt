# Copyright (c) 2024 Nordic Semiconductor ASA
# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BUILD_WITH_TFA)

  if(CONFIG_TFA_MAKE_BUILD_TYPE_DEBUG)
    set(TFA_BUILD_TYPE "debug")
  else()
    set(TFA_BUILD_TYPE "release")
  endif()

  set(TFA_BINARY_DIR ${CMAKE_BINARY_DIR}/tfa)
  set(TFA_ARTIFACT_DIR "${TFA_BINARY_DIR}/${CONFIG_TFA_PLATFORM}/${CONFIG_TFA_TARGET_BOARD}/${TFA_BUILD_TYPE}")
endif()

if(CONFIG_TI_K3_BUILD_TIBOOT3)

  # Board config file paths (from Kconfig, defaults to ${BOARD_DIR}/support)
  set(TI_K3_BOARD_CFG_SCHEMA "${BOARD_DIR}/${CONFIG_TI_K3_BOARD_CONFIG_DIR}/schema.yaml")
  set(TI_K3_BOARD_CFG_YAML "${BOARD_DIR}/${CONFIG_TI_K3_BOARD_CONFIG_DIR}/board-cfg.yaml")
  set(TI_K3_SEC_CFG_YAML "${BOARD_DIR}/${CONFIG_TI_K3_BOARD_CONFIG_DIR}/sec-cfg.yaml")
  set(TI_K3_PM_CFG_YAML "${BOARD_DIR}/${CONFIG_TI_K3_BOARD_CONFIG_DIR}/pm-cfg.yaml")
  set(TI_K3_RM_CFG_YAML "${BOARD_DIR}/${CONFIG_TI_K3_BOARD_CONFIG_DIR}/rm-cfg.yaml")

  set(TI_K3_TIFS_BIN "${ZEPHYR_HAL_TI_MODULE_DIR}/zephyr/blobs/ti-sysfw/${CONFIG_TI_K3_TIFS_BIN}")
  set(TI_K3_TIFS_CERT_BIN "${ZEPHYR_HAL_TI_MODULE_DIR}/zephyr/blobs/ti-sysfw/${CONFIG_TI_K3_TIFS_CERT_BIN}")

  # Validate required files
  set(TI_K3_MISSING "")
  if(NOT EXISTS ${TI_K3_TIFS_BIN})
    list(APPEND TI_K3_MISSING "TIFS: ${CONFIG_TI_K3_TIFS_BIN}")
  endif()
  if(NOT EXISTS ${TI_K3_TIFS_CERT_BIN})
    list(APPEND TI_K3_MISSING "TIFS cert: ${CONFIG_TI_K3_TIFS_CERT_BIN}")
  endif()
  if(NOT EXISTS "${CONFIG_TI_K3_CERT_KEY_PATH}")
    list(APPEND TI_K3_MISSING "signing key: ${CONFIG_TI_K3_CERT_KEY_PATH}")
  endif()
  if(NOT EXISTS "${TI_K3_BOARD_CFG_SCHEMA}")
    list(APPEND TI_K3_MISSING "schema: ${TI_K3_BOARD_CFG_SCHEMA}")
  endif()
  if(NOT EXISTS "${TI_K3_BOARD_CFG_YAML}")
    list(APPEND TI_K3_MISSING "board-cfg: ${TI_K3_BOARD_CFG_YAML}")
  endif()
  if(CONFIG_TI_K3_REQUIRES_SEC_CFG AND NOT EXISTS "${TI_K3_SEC_CFG_YAML}")
    list(APPEND TI_K3_MISSING "sec-cfg: ${TI_K3_SEC_CFG_YAML}")
  endif()
  if(CONFIG_TI_K3_REQUIRES_PM_CFG AND NOT EXISTS "${TI_K3_PM_CFG_YAML}")
    list(APPEND TI_K3_MISSING "pm-cfg: ${TI_K3_PM_CFG_YAML}")
  endif()
  if(CONFIG_TI_K3_REQUIRES_RM_CFG AND NOT EXISTS "${TI_K3_RM_CFG_YAML}")
    list(APPEND TI_K3_MISSING "rm-cfg: ${TI_K3_RM_CFG_YAML}")
  endif()

  if(TI_K3_MISSING)
    string(REPLACE ";" "\n  - " TI_K3_MISSING_STR "${TI_K3_MISSING}")
    message(WARNING
      "TI K3: Missing files for tiboot3.bin generation:\n  - ${TI_K3_MISSING_STR}\n"
      "tiboot3.bin will NOT be generated. Run 'west blobs fetch hal_ti' to fetch firmware.")
  else()
    # Output paths
    set(TI_K3_COMBINED_BOARDCFG "${CMAKE_BINARY_DIR}/zephyr/tiboot3-boardcfg.bin")
    set(TI_K3_TIBOOT3_BIN "${CMAKE_BINARY_DIR}/zephyr/tiboot3.bin")

    # Build combined board config blob command
    set(TI_K3_BOARDCFG_CMD
      ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/ti_board_config.py
      --schema ${TI_K3_BOARD_CFG_SCHEMA}
      --board-cfg ${TI_K3_BOARD_CFG_YAML}
    )
    if(CONFIG_TI_K3_REQUIRES_SEC_CFG)
      list(APPEND TI_K3_BOARDCFG_CMD --sec-cfg ${TI_K3_SEC_CFG_YAML})
    endif()
    if(CONFIG_TI_K3_REQUIRES_PM_CFG)
      list(APPEND TI_K3_BOARDCFG_CMD --pm-cfg ${TI_K3_PM_CFG_YAML})
    endif()
    if(CONFIG_TI_K3_REQUIRES_RM_CFG)
      list(APPEND TI_K3_BOARDCFG_CMD --rm-cfg ${TI_K3_RM_CFG_YAML})
    endif()
    list(APPEND TI_K3_BOARDCFG_CMD --output ${TI_K3_COMBINED_BOARDCFG})

    # Generate combined board config blob
    add_custom_command(
      OUTPUT ${TI_K3_COMBINED_BOARDCFG}
      COMMAND ${TI_K3_BOARDCFG_CMD}
      DEPENDS ${TI_K3_BOARD_CFG_YAML} ${TI_K3_BOARD_CFG_SCHEMA}
      COMMENT "Generating tiboot3.bin board config blob"
    )

    if(CONFIG_BUILD_WITH_TFA)
      set(TI_K3_SPL_BIN "${TFA_ARTIFACT_DIR}/${CONFIG_TI_K3_SPL_BIN}")
    else()
      set(TI_K3_SPL_BIN "${CMAKE_BINARY_DIR}/zephyr/${CONFIG_TI_K3_SPL_BIN}")
    endif()

    # Set dependencies (TFA required for AM62L, enforced by Kconfig)
    set(TI_K3_TIBOOT3_DEPENDS ${TI_K3_COMBINED_BOARDCFG})
    if(CONFIG_BUILD_WITH_TFA)
      list(APPEND TI_K3_TIBOOT3_DEPENDS tfa)
    endif()

    # Generate tiboot3.bin
    add_custom_command(
      OUTPUT ${TI_K3_TIBOOT3_BIN}
      COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/rom_image_gen.py
        --swrv ${CONFIG_TI_K3_SW_REVISION}
        --spl-bin ${TI_K3_SPL_BIN}
        --sysfw-bin ${TI_K3_TIFS_BIN}
        --sysfw-inner-cert ${TI_K3_TIFS_CERT_BIN}
        --boardcfg-blob ${TI_K3_COMBINED_BOARDCFG}
        --spl-loadaddr ${CONFIG_TI_K3_SPL_LOAD_ADDRESS}
        --sysfw-loadaddr ${CONFIG_TI_K3_TIFS_LOAD_ADDRESS}
        --bcfg-loadaddr ${CONFIG_TI_K3_BOARDCFG_LOAD_ADDRESS}
        --key ${CONFIG_TI_K3_CERT_KEY_PATH}
        --rom-image ${TI_K3_TIBOOT3_BIN}
        --dual-stage-boot
      DEPENDS ${TI_K3_TIBOOT3_DEPENDS}
      COMMENT "Generating tiboot3.bin"
    )

    add_custom_target(tiboot3 ALL DEPENDS ${TI_K3_TIBOOT3_BIN})

  endif()

endif()

if(CONFIG_TI_K3_BUILD_TISPL)

  # Board config file paths
  set(TI_K3_BOARD_CFG_SCHEMA "${BOARD_DIR}/${CONFIG_TI_K3_BOARD_CONFIG_DIR}/schema.yaml")
  set(TI_K3_BOARD_CFG_YAML "${BOARD_DIR}/${CONFIG_TI_K3_BOARD_CONFIG_DIR}/board-cfg.yaml")
  set(TI_K3_SEC_CFG_YAML "${BOARD_DIR}/${CONFIG_TI_K3_BOARD_CONFIG_DIR}/sec-cfg.yaml")
  set(TI_K3_PM_CFG_YAML "${BOARD_DIR}/${CONFIG_TI_K3_BOARD_CONFIG_DIR}/pm-cfg.yaml")
  set(TI_K3_RM_CFG_YAML "${BOARD_DIR}/${CONFIG_TI_K3_BOARD_CONFIG_DIR}/rm-cfg.yaml")

  # TIFS firmware paths (same as tiboot3)
  set(TI_K3_TIFS_BIN "${ZEPHYR_HAL_TI_MODULE_DIR}/zephyr/blobs/ti-sysfw/${CONFIG_TI_K3_TIFS_BIN}")
  set(TI_K3_TIFS_CERT_BIN "${ZEPHYR_HAL_TI_MODULE_DIR}/zephyr/blobs/ti-sysfw/${CONFIG_TI_K3_TIFS_CERT_BIN}")

  # Validate required files
  set(TI_K3_MISSING "")
  if(NOT EXISTS ${TI_K3_TIFS_BIN})
    list(APPEND TI_K3_MISSING "TIFS: ${CONFIG_TI_K3_TIFS_BIN}")
  endif()
  if(NOT EXISTS ${TI_K3_TIFS_CERT_BIN})
    list(APPEND TI_K3_MISSING "TIFS cert: ${CONFIG_TI_K3_TIFS_CERT_BIN}")
  endif()
  if(NOT EXISTS "${CONFIG_TI_K3_CERT_KEY_PATH}")
    list(APPEND TI_K3_MISSING "signing key: ${CONFIG_TI_K3_CERT_KEY_PATH}")
  endif()
  if(NOT EXISTS "${TI_K3_BOARD_CFG_SCHEMA}")
    list(APPEND TI_K3_MISSING "schema: ${TI_K3_BOARD_CFG_SCHEMA}")
  endif()
  if(NOT EXISTS "${TI_K3_BOARD_CFG_YAML}")
    list(APPEND TI_K3_MISSING "board-cfg: ${TI_K3_BOARD_CFG_YAML}")
  endif()
  if(CONFIG_TI_K3_REQUIRES_SEC_CFG AND NOT EXISTS "${TI_K3_SEC_CFG_YAML}")
    list(APPEND TI_K3_MISSING "sec-cfg: ${TI_K3_SEC_CFG_YAML}")
  endif()
  if(CONFIG_TI_K3_REQUIRES_PM_CFG AND NOT EXISTS "${TI_K3_PM_CFG_YAML}")
    list(APPEND TI_K3_MISSING "pm-cfg: ${TI_K3_PM_CFG_YAML}")
  endif()
  if(CONFIG_TI_K3_REQUIRES_RM_CFG AND NOT EXISTS "${TI_K3_RM_CFG_YAML}")
    list(APPEND TI_K3_MISSING "rm-cfg: ${TI_K3_RM_CFG_YAML}")
  endif()

  if(TI_K3_MISSING)
    string(REPLACE ";" "\n  - " TI_K3_MISSING_STR "${TI_K3_MISSING}")
    message(WARNING
      "TI K3: Missing files for tispl.bin generation:\n  - ${TI_K3_MISSING_STR}\n"
      "tispl.bin will NOT be generated. Run 'west blobs fetch hal_ti' to fetch firmware.")
  else()
    # Output paths
    set(TI_K3_TISPL_BOARDCFG "${CMAKE_BINARY_DIR}/zephyr/tispl-boardcfg.bin")
    set(TI_K3_TISPL_BIN "${CMAKE_BINARY_DIR}/zephyr/tispl.bin")

    # Build combined board config blob command (same as tiboot3)
    set(TI_K3_TISPL_BOARDCFG_CMD
      ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/ti_board_config.py
      --schema ${TI_K3_BOARD_CFG_SCHEMA}
      --board-cfg ${TI_K3_BOARD_CFG_YAML}
    )
    if(CONFIG_TI_K3_REQUIRES_SEC_CFG)
      list(APPEND TI_K3_TISPL_BOARDCFG_CMD --sec-cfg ${TI_K3_SEC_CFG_YAML})
    endif()
    if(CONFIG_TI_K3_REQUIRES_PM_CFG)
      list(APPEND TI_K3_TISPL_BOARDCFG_CMD --pm-cfg ${TI_K3_PM_CFG_YAML})
    endif()
    if(CONFIG_TI_K3_REQUIRES_RM_CFG)
      list(APPEND TI_K3_TISPL_BOARDCFG_CMD --rm-cfg ${TI_K3_RM_CFG_YAML})
    endif()
    list(APPEND TI_K3_TISPL_BOARDCFG_CMD --output ${TI_K3_TISPL_BOARDCFG})

    set(TI_K3_ATF_BIN "${TFA_ARTIFACT_DIR}/bl31.bin")
    set(TI_K3_APP_BIN "${CMAKE_BINARY_DIR}/zephyr/zephyr.bin")

    # Generate tispl board config blob
    add_custom_command(
      OUTPUT ${TI_K3_TISPL_BOARDCFG}
      COMMAND ${TI_K3_TISPL_BOARDCFG_CMD}
      DEPENDS ${TI_K3_BOARD_CFG_YAML} ${TI_K3_BOARD_CFG_SCHEMA}
      COMMENT "Generating tispl.bin board config blob"
    )

    # Generate tispl.bin (depends on TFA artifacts and zephyr.bin)
    add_custom_command(
      OUTPUT ${TI_K3_TISPL_BIN}
      COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/rom_image_gen.py
        --swrv ${CONFIG_TI_K3_SW_REVISION}
        --spl-bin ${TI_K3_ATF_BIN}
        --spl-loadaddr ${CONFIG_TI_K3_TFA_LOAD_ADDRESS}
        --sysfw-bin ${TI_K3_TIFS_BIN}
        --sysfw-loadaddr ${CONFIG_TI_K3_TIFS_LOAD_ADDRESS}
        --sysfw-inner-cert ${TI_K3_TIFS_CERT_BIN}
        --boardcfg-blob ${TI_K3_TISPL_BOARDCFG}
        --bcfg-loadaddr ${CONFIG_TI_K3_BOARDCFG_LOAD_ADDRESS}
        --application-bin ${TI_K3_APP_BIN}
        --application-loadaddr ${CONFIG_TI_K3_ZEPHYR_APP_BIN_LOAD_ADDRESS}
        --key ${CONFIG_TI_K3_CERT_KEY_PATH}
        --rom-image ${TI_K3_TISPL_BIN}
        --dual-stage-boot
      DEPENDS
        tfa
        ${TI_K3_APP_BIN}
        ${TI_K3_TISPL_BOARDCFG}
      COMMENT "Generating tispl.bin"
    )

    # Wrapper target to ensure tispl.bin is built
    add_custom_target(tispl ALL DEPENDS ${TI_K3_TISPL_BIN})

  endif()
endif()


add_subdirectory(./common)
