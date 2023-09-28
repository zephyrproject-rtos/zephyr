#
# Copyright (c) 2023 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#
if(CONFIG_WEB3_IOT_SDK)

if("${ARCH}" STREQUAL "x86")
    if( NOT CONFIG_X86_64)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    endif()
endif()
get_filename_component(MODULE_NAME ${ZEPHYR_CURRENT_MODULE_DIR} NAME)
set(WEB3_IOT_SDK_BINARY_DIR ${CMAKE_BINARY_DIR}/modules/${MODULE_NAME})
file(MAKE_DIRECTORY ${WEB3_IOT_SDK_BINARY_DIR})


set(WS_IOTEX_SDK_INTERFACE ${WEB3_IOT_SDK_BINARY_DIR}/component/services/iotex-f/interface/libiotex_sdk_interface.a)
set(WS_IOTEX_SDK_TFM_SERVICE ${WEB3_IOT_SDK_BINARY_DIR}/component/services/iotex-f/secure_fw/services/audit_logging/libtfm_service_audit.a)
set(WS_IOTEX_SDK_TFM_PSA_ROT_CRYPTO ${WEB3_IOT_SDK_BINARY_DIR}/component/services/iotex-f/secure_fw/services/crypto/libtfm_psa_rot_partition_crypto.a)
set(WS_IOTEX_SDK_TFM_SPM_CORE ${WEB3_IOT_SDK_BINARY_DIR}/component/services/iotex-f/secure_fw/core/libtfm_spm_core.a)
set(WS_IOTEX_SDK_PARTITION  ${ZEPHYR_CURRENT_MODULE_DIR}/component/services/iotex-f)
set(WS_IOTEX_SDK_INCLUDE ${ZEPHYR_WEB3_IOT_SDK_MODULE_DIR}/include CACHE PATH "web3 sdk include path" FORCE)

target_include_directories(mbedTLS INTERFACE
    ${ZEPHYR_WEB3_IOT_SDK_MODULE_DIR}/component/services/iotex-f/secure_fw/services/crypto
)

file(GLOB_RECURSE USER_CONFIG_FILE "${CMAKE_SOURCE_DIR}/${CONFIG_MBEDTLS_USER_CONFIG_FILE}")
get_filename_component(MBEDTLS_USER_CONFIG_FILE_DIR ${USER_CONFIG_FILE} DIRECTORY)

set(EXTERN_MBEDTLS_CONFIG ${CMAKE_BINARY_DIR}/zephyr/include/generated/autoconf.h)
include_directories(${MBEDTLS_USER_CONFIG_FILE_DIR})
add_subdirectory(${WS_IOTEX_SDK_PARTITION}   component/services/iotex-f)

zephyr_library_link_libraries(
    ${WS_IOTEX_SDK_INTERFACE}
    ${WS_IOTEX_SDK_TFM_PSA_ROT_CRYPTO}
    ${WS_IOTEX_SDK_TFM_SPM_CORE}
)

endif()



