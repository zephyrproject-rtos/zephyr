# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bt_crypto.c)

if(CONFIG_BT_USE_PSA_API)
    zephyr_library_sources(bt_crypto_psa.c)
    zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
    zephyr_library_include_directories_ifdef(CONFIG_BUILD_WITH_TFM
        $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/interface/include
    )
else()
    zephyr_library_sources(bt_crypto_tc.c)
endif()

if(CONFIG_BT_CRYPTO_LOG_LEVEL_DBG)
    message(WARNING "CONFIG_BT_CRYPTO_LOG_LEVEL_DBG is enabled.
    Private security keys such as the Long Term Key will be printed out.
    Do not use in production."
    )
endif()
