# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_IRONSIDE_INTEGRATION_KIT_MODULE_DIR}/client/include)
zephyr_include_directories(include)

zephyr_library()
zephyr_library_sources(
  ${ZEPHYR_IRONSIDE_INTEGRATION_KIT_MODULE_DIR}/client/src/client.c
  ${ZEPHYR_IRONSIDE_INTEGRATION_KIT_MODULE_DIR}/client/src/services.c
  client_lock.c
  layout_check.c
)
zephyr_library_sources_ifdef(CONFIG_IRONSIDE_TE_LOG_LEVEL
  log.c
)
zephyr_library_sources_ifdef(CONFIG_IRONSIDE_TE_FAULT_LOGGING
  boot_fault_log.c
)
zephyr_library_sources_ifdef(CONFIG_IRONSIDE_TE_BOOT_LAYOUT_CHECK
  boot_layout_check.c
)

if(CONFIG_PSA_CRYPTO_PROVIDER_IRONSIDE_TE)
  zephyr_library_sources(
    ${ZEPHYR_IRONSIDE_INTEGRATION_KIT_MODULE_DIR}/client/src/psa_crypto.c
    psa_crypto_abi_check.c
  )

  if(NOT CONFIG_TF_PSA_CRYPTO)
    zephyr_include_directories(
      ${CONFIG_TF_PSA_CRYPTO_MODULE_DIR}/include
      ${CONFIG_TF_PSA_CRYPTO_MODULE_DIR}/drivers/builtin/include
      ${CONFIG_TF_PSA_CRYPTO_DISPATCH_DIR}/include
      ${ZEPHYR_BASE}/modules/mbedtls/configs
    )

    zephyr_compile_definitions(
      TF_PSA_CRYPTO_CONFIG_FILE="${CONFIG_TF_PSA_CRYPTO_CONFIG_FILE}"
    )
  endif()
endif()

# IronSide TE only boots a signed application. Sysbuild may still point the
# image at another script through SIGNING_SCRIPT, which takes precedence.
set_target_properties(zephyr_property_target PROPERTIES
  SIGNING_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/signing.cmake
)
