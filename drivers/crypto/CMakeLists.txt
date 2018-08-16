zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_CRYPTO_TINYCRYPT_SHIM} crypto_tc_shim.c
            IFDEF:${CONFIG_CRYPTO_ATAES132A}      crypto_ataes132a.c
            IFDEF:${CONFIG_CRYPTO_MBEDTLS_SHIM}   crypto_mtls_shim.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

if(CONFIG_MBEDTLS)
  target_link_libraries(drivers PRIVATE mbedTLS)
endif()
