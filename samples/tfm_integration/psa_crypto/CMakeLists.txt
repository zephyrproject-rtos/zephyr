# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tfm_psa_crypto)

target_sources(app PRIVATE
  src/main.c
  src/psa_attestation.c
  src/psa_crypto.c
  src/shell.c
  src/util_app_cfg.c
  src/util_app_log.c
  src/util_sformat.c
  )

zephyr_library_link_libraries(tfm_api)

# In TF-M, default value of CRYPTO_ENGINE_BUF_SIZE is 0x2080. It causes
# insufficient memory failure while verifying signature. Increase it to 0x2400.
set_property(TARGET zephyr_property_target
             APPEND PROPERTY TFM_CMAKE_OPTIONS
             -DCRYPTO_ENGINE_BUF_SIZE=0x2400
)

zephyr_include_directories(src/tls_config)
