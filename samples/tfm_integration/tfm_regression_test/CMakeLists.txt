#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tfm_regression_test)

set(TFM_TEST_REG_TEST_PATH  ${ZEPHYR_TF_M_TESTS_MODULE_DIR}/tests_reg/test)
set(TFM_TEST_LIB_PATH  ${ZEPHYR_TF_M_TESTS_MODULE_DIR}/lib)
set(TFM_API_NS_PATH ${CMAKE_BINARY_DIR}/tfm/api_ns)

macro(add_inc_and_src_if_def)
  cmake_parse_arguments(
    ARG
    "" # options - none here
    "KCONFIG" # one_value_keywords
    "INC;SRC;GLOB_SRC" # multi_value_keywords
    ${ARGN}
  )

  if(${ARG_KCONFIG})
    # Append include directories
    target_include_directories(app PRIVATE ${ARG_INC})
    # Append source files
    file(GLOB new_sources ${ARG_GLOB_SRC})
    list(APPEND new_sources ${ARG_SRC})
    target_sources(app PRIVATE ${new_sources})
  endif()
endmacro()

message(STATUS "Enabled:")

macro(def_if_def kconfig build_flag)
  if(${kconfig})
    target_compile_definitions(app PRIVATE -D${build_flag})
    message(STATUS " ${build_flag}")
  endif()
endmacro()

target_sources(app PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
)

add_inc_and_src_if_def(
  KCONFIG
    TRUE # Not a real Kconfig, because we always need the following
  GLOB_SRC
    ${TFM_TEST_REG_TEST_PATH}/secure_regression/*.c
    ${TFM_TEST_REG_TEST_PATH}/ns_regression/*.c
    ${TFM_TEST_REG_TEST_PATH}/framework/*.c
  SRC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/common/suites/client_api_tests.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/common/suites/irq_test.c
  INC
    ${TFM_TEST_LIB_PATH}/log
    ${TFM_TEST_LIB_PATH}/os_wrapper/
    ${TFM_TEST_REG_TEST_PATH}/ns_regression
    ${TFM_TEST_REG_TEST_PATH}/secure_regression
    ${TFM_TEST_REG_TEST_PATH}/framework
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/common_test_services/tfm_secure_client_2
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/common/suites
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/common/service
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/common
    ${TFM_API_NS_PATH}/platform/include
    ${TFM_API_NS_PATH}/interface/include
)

add_inc_and_src_if_def(
  KCONFIG
    CONFIG_TFM_PARTITION_CRYPTO
  INC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/crypto
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/crypto/non_secure
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/crypto/secure
  GLOB_SRC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/crypto/*.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/crypto/non_secure/*.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/crypto/secure/*.c
)
def_if_def(CONFIG_TFM_PARTITION_CRYPTO                  TEST_NS_CRYPTO)
def_if_def(CONFIG_TFM_PARTITION_CRYPTO                  TEST_S_CRYPTO)
def_if_def(CONFIG_TFM_PARTITION_CRYPTO                  TFM_CRYPTO_TEST_UNSUPPORTED_ALG)
def_if_def(CONFIG_TFM_PARTITION_CRYPTO                  TFM_CRYPTO_TEST_SINGLE_PART_FUNCS)
def_if_def(CONFIG_PSA_WANT_ALG_CBC_NO_PADDING           TFM_CRYPTO_TEST_ALG_CBC)
def_if_def(CONFIG_PSA_WANT_ALG_CCM                      TFM_CRYPTO_TEST_ALG_CCM)
def_if_def(CONFIG_PSA_WANT_ALG_CFB                      TFM_CRYPTO_TEST_ALG_CFB)
def_if_def(CONFIG_PSA_WANT_ALG_ECB_NO_PADDING           TFM_CRYPTO_TEST_ALG_ECB)
def_if_def(CONFIG_PSA_WANT_ALG_CTR                      TFM_CRYPTO_TEST_ALG_CTR)
def_if_def(CONFIG_PSA_WANT_ALG_OFB                      TFM_CRYPTO_TEST_ALG_OFB)
def_if_def(CONFIG_PSA_WANT_ALG_GCM                      TFM_CRYPTO_TEST_ALG_GCM)
def_if_def(CONFIG_PSA_WANT_ALG_SHA_224                  TFM_CRYPTO_TEST_ALG_SHA_224)
def_if_def(CONFIG_PSA_WANT_ALG_SHA_384                  TFM_CRYPTO_TEST_ALG_SHA_384)
def_if_def(CONFIG_PSA_WANT_ALG_SHA_512                  TFM_CRYPTO_TEST_ALG_SHA_512)
def_if_def(CONFIG_PSA_WANT_ALG_HKDF                     TFM_CRYPTO_TEST_HKDF)
def_if_def(CONFIG_PSA_WANT_ALG_ECDH                     TFM_CRYPTO_TEST_ECDH)
def_if_def(CONFIG_PSA_WANT_ALG_RSA_PSS                  TFM_CRYPTO_TEST_ALG_RSASSA_PSS_VERIFICATION)
def_if_def(CONFIG_PSA_WANT_KEY_TYPE_CHACHA20            TFM_CRYPTO_TEST_CHACHA20)
def_if_def(CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305        TFM_CRYPTO_TEST_ALG_CHACHA20_POLY1305)
def_if_def(CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA      TFM_CRYPTO_TEST_ALG_DETERMINISTIC_ECDSA)
def_if_def(CONFIG_PSA_WANT_ALG_ECDSA                    TFM_CRYPTO_TEST_ALG_ECDSA)

add_inc_and_src_if_def(
  KCONFIG
    CONFIG_TFM_PARTITION_PROTECTED_STORAGE
  INC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/ps/non_secure
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/ps/secure
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/ps/secure/nv_counters
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/ps/service/
    ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/secure_fw/partitions/protected_storage
  GLOB_SRC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/ps/non_secure/*.c
  SRC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/ps/secure/psa_ps_s_interface_testsuite.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/ps/secure/psa_ps_s_reliability_testsuite.c
)
def_if_def(CONFIG_TFM_PARTITION_PROTECTED_STORAGE         TEST_NS_PS)
def_if_def(CONFIG_TFM_PARTITION_PROTECTED_STORAGE         TEST_S_PS)

add_inc_and_src_if_def(
  KCONFIG
    CONFIG_TFM_PS_ROLLBACK_PROTECTION
  SRC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/ps/secure/ps_rollback_protection_testsuite.c
)
def_if_def(CONFIG_TFM_PS_ROLLBACK_PROTECTION              PS_TEST_NV_COUNTERS)
def_if_def(CONFIG_TFM_PS_ROLLBACK_PROTECTION              PLATFORM_DEFAULT_NV_COUNTERS)

add_inc_and_src_if_def(
  KCONFIG
    CONFIG_TFM_PARTITION_INTERNAL_TRUSTED_STORAGE
  INC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/its/non_secure
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/its/secure
  GLOB_SRC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/its/non_secure/*.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/its/secure/*.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/its/*.c
)
def_if_def(CONFIG_TFM_PARTITION_INTERNAL_TRUSTED_STORAGE  TEST_NS_ITS)
def_if_def(CONFIG_TFM_PARTITION_INTERNAL_TRUSTED_STORAGE  TEST_S_ITS)
def_if_def(CONFIG_TFM_PARTITION_INTERNAL_TRUSTED_STORAGE  TFM_PARTITION_INTERNAL_TRUSTED_STORAGE)
def_if_def(CONFIG_TFM_PARTITION_INTERNAL_TRUSTED_STORAGE  TFM_INTERNAL_TRUSTED_STORAGE_SERVICE)

add_inc_and_src_if_def(
  KCONFIG
    CONFIG_TFM_PARTITION_PLATFORM
  INC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/platform/non_secure
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/platform/secure
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/platform/
  GLOB_SRC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/platform/non_secure/*.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/platform/secure/*.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/platform/*.c
)
def_if_def(CONFIG_TFM_PARTITION_PLATFORM                  TEST_NS_PLATFORM)
def_if_def(CONFIG_TFM_PARTITION_PLATFORM                  TEST_S_PLATFORM)

add_inc_and_src_if_def(
  KCONFIG
    CONFIG_TFM_PARTITION_INITIAL_ATTESTATION
  INC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/attestation/non_secure
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/attestation/secure
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/attestation
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/attestation/ext/qcbor_util
    ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/secure_fw/partitions/initial_attestation
    ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/secure_fw/spm/include/boot
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/inc
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/qcbor-src/inc
  SRC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/attestation/non_secure/attest_asymmetric_ns_interface_testsuite.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/attestation/secure/attest_asymmetric_s_interface_testsuite.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/attestation/attest_token_decode_asymmetric.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/attestation/attest_token_decode_common.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/attestation/attest_token_test.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/qcbor-src/src/ieee754.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/qcbor-src/src/qcbor_decode.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/qcbor-src/src/qcbor_encode.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/qcbor-src/src/qcbor_err_to_str.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/qcbor-src/src/UsefulBuf.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_encrypt_dec.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_encrypt_enc.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_key.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_mac_compute.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_mac_validate.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_parameters.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_qcbor_gap.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_recipient_dec_esdh.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_recipient_dec_keywrap.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_recipient_enc_esdh.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_recipient_enc_keywrap.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_sign1_sign.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_sign1_verify.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_signature_sign_eddsa.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_signature_sign_main.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_signature_sign_restart.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_signature_verify_eddsa.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_signature_verify_main.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_sign_sign.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_sign_verify.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_util.c
)
set_source_files_properties(
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/qcbor-src/src/ieee754.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/qcbor-src/src/qcbor_decode.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/qcbor-src/src/qcbor_encode.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/qcbor-src/src/qcbor_err_to_str.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/qcbor-src/src/UsefulBuf.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_encrypt_dec.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_encrypt_enc.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_key.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_mac_compute.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_mac_validate.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_parameters.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_qcbor_gap.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_recipient_dec_esdh.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_recipient_dec_keywrap.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_recipient_enc_esdh.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_recipient_enc_keywrap.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_sign1_sign.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_sign1_verify.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_signature_sign_eddsa.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_signature_sign_main.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_signature_sign_restart.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_signature_verify_eddsa.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_signature_verify_main.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_sign_sign.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_sign_verify.c
    ${CMAKE_BINARY_DIR}/tfm/lib/ext/t_cose-src/src/t_cose_util.c
    PROPERTIES GENERATED TRUE
)
def_if_def(CONFIG_TFM_PARTITION_INITIAL_ATTESTATION       TEST_NS_ATTESTATION)
def_if_def(CONFIG_TFM_PARTITION_INITIAL_ATTESTATION       TEST_S_ATTESTATION)

add_inc_and_src_if_def(
  KCONFIG
    CONFIG_TFM_PARTITION_FIRMWARE_UPDATE
  INC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/fwu/mcuboot/non_secure
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/fwu/mcuboot/secure
  GLOB_SRC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/fwu/mcuboot/non_secure/*.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/fwu/mcuboot/secure/*.c
)
def_if_def(CONFIG_TFM_PARTITION_FIRMWARE_UPDATE           TEST_NS_FWU)
def_if_def(CONFIG_TFM_PARTITION_FIRMWARE_UPDATE           TEST_S_FWU)

add_inc_and_src_if_def(
  KCONFIG
    CONFIG_TFM_IPC
  INC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/ipc/non_secure
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/ipc/secure
  GLOB_SRC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/ipc/non_secure/*.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/ipc/secure/*.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/ipc/*.c
)
def_if_def(CONFIG_TFM_IPC                                 TEST_NS_IPC)
def_if_def(CONFIG_TFM_IPC                                 TEST_S_IPC)

add_inc_and_src_if_def(
  KCONFIG
    CONFIG_TFM_SFN
  INC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/sfn/non_secure
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/sfn/secure
  GLOB_SRC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/sfn/non_secure/*.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/sfn/secure/*.c
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/sfn/*.c
)
def_if_def(CONFIG_TFM_SFN                                 TEST_NS_SFN_BACKEND)
def_if_def(CONFIG_TFM_SFN                                 TEST_S_SFN_BACKEND)

add_inc_and_src_if_def(
  KCONFIG
    CONFIG_TFM_PROFILE_TYPE_SMALL
  SRC
    ${TFM_TEST_REG_TEST_PATH}/secure_fw/suites/spm/common/suites/mmiovec_test.c
)
