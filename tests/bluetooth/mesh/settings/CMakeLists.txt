# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth_mesh_settings)

file(GLOB app_sources src/*.c)
target_sources(app
  PRIVATE
  ${app_sources}
  ${ZEPHYR_BASE}/subsys/bluetooth/mesh/settings.c
)

target_include_directories(app
  PRIVATE
  ${ZEPHYR_BASE}/subsys/bluetooth/mesh
  ${ZEPHYR_BASE}/subsys/bluetooth
  ${CONFIG_TF_PSA_CRYPTO_MODULE_DIR}/include
  ${CONFIG_TF_PSA_CRYPTO_MODULE_DIR}/drivers/builtin/include/
  ${CONFIG_TF_PSA_CRYPTO_MODULE_DIR}/dispatch/include/
)

# The two timeouts must differ, otherwise the deadlines the tests distinguish collapse.
target_compile_options(app
  PRIVATE
  -DCONFIG_BT_MESH_STORE_TIMEOUT=2
  -DCONFIG_BT_MESH_RPL_STORE_TIMEOUT=10
  -DCONFIG_BT_MESH_RPL_STORAGE_MODE_SETTINGS=1
  -DCONFIG_BT_MESH_CDB=1
  -DCONFIG_BT_MESH_CDB_NODE_COUNT=1
  -DCONFIG_BT_MESH_CDB_SUBNET_COUNT=1
  -DCONFIG_BT_MESH_CDB_APP_KEY_COUNT=1
  -DCONFIG_BT_MESH_OD_PRIV_PROXY_SRV=1
  -DCONFIG_BT_MESH_PROXY_SOLICITATION=1
  -DCONFIG_BT_MESH_BRG_CFG_SRV=1
  -DCONFIG_BT_SETTINGS=1
  -DCONFIG_PSA_CRYPTO_PROVIDER_MBEDTLS
)
