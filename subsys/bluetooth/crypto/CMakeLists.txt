# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bt_crypto.c)

if(CONFIG_BT_CRYPTO_LOG_LEVEL_DBG)
    message(WARNING "CONFIG_BT_CRYPTO_LOG_LEVEL_DBG is enabled.
    Private security keys such as the Long Term Key will be printed out.
    Do not use in production."
    )
endif()
