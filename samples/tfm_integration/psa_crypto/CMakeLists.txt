# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tfm_psa_crypto)

# Source files in this sample
target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/psa_attestation.c)
target_sources(app PRIVATE src/psa_crypto.c)
target_sources(app PRIVATE src/shell.c)
target_sources(app PRIVATE src/util_app_cfg.c)
target_sources(app PRIVATE src/util_app_log.c)
target_sources(app PRIVATE src/util_sformat.c)

target_include_directories(app PRIVATE
  $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/install/interface/include
)

# In TF-M, default value of CRYPTO_ENGINE_BUF_SIZE is 0x2080. It causes
# insufficient memory failure while verifying signature. Increase it to 0x2400.
set_property(TARGET zephyr_property_target
             APPEND PROPERTY TFM_CMAKE_OPTIONS
             -DCRYPTO_ENGINE_BUF_SIZE=0x2400
)

zephyr_include_directories(${APPLICATION_SOURCE_DIR}/src/tls_config)
