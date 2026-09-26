# SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(psa_crypto_test_suite)


if(NOT CONFIG_TF_PSA_CRYPTO_TEST_BUILD_PATH)
  # TODO: find a way to generate fiies in CMAKE_CURRENT_BINARY_DIR.
  # It seems tf_psa_crypto expects the build path in the source repo.
  #set(tf_psa_crypto_build_path ${CMAKE_CURRENT_BINARY_DIR}/build_tf_psa_crypto)
  set(tf_psa_crypto_build_path ${CONFIG_TF_PSA_CRYPTO_PATH}/build)
else()
  set(tf_psa_crypto_build_path ${CONFIG_TF_PSA_CRYPTO_TEST_BUILD_PATH})
endif()
set(tf_psa_crypto_build_tests_path ${tf_psa_crypto_build_path}/tests)

if(NOT CONFIG_MBEDTLS_FRAMEWORK_PATH)
  set(mbdtls_framwork_path ${CONFIG_TF_PSA_CRYPTO_PATH}/framework)
else()
  set(mbdtls_framwork_path ${CONFIG_MBEDTLS_FRAMEWORK_PATH})
 endif()

set(supported_test_suites
  test_suite_psa_crypto
  test_suite_psa_crypto_hash
)

set(generated_test_sources
  ${tf_psa_crypto_build_tests_path}/zephyr_test.c
)

foreach(test_suite ${supported_test_suites})
  list(APPEND generated_test_sources
    ${tf_psa_crypto_build_tests_path}/${test_suite}.c
  )
endforeach()

function(generate_test_suite_source test_suite_list_file)
  FILE(GLOB tf_psa_crypto_test_src_templates ${CONFIG_TF_PSA_CRYPTO_PATH}/tests/suites/*.function)
  FILE(GLOB tf_psa_crypto_test_data_files ${CONFIG_TF_PSA_CRYPTO_PATH}/tests/suites/*.data)
  FILE(GLOB tf_psa_crypto_test_xdata_files ${tf_psa_crypto_build_tests_path}/tests/suites/*.datax)

  add_custom_command(
    OUTPUT
        ${generated_test_sources}
    # FIXME: genrate tf-osa-crypto tests files from this command.
    # Currently fails, hence the commented-out lines below.
    # One needs to configure and build a tf-psa-crypto source tree
    # to get some pregenerated file. Below we only rebuild the
    # test_suite we're interrested in.
    #COMMAND
    #    ${CMAKE_COMMAND} -S ${CONFIG_TF_PSA_CRYPTO_PATH} -B ${tf_psa_crypto_build_path}
    COMMAND
        #make -C ${tf_psa_crypto_build_path} ${supported_test_suites}
        ${CMAKE_COMMAND} --build ${tf_psa_crypto_build_path} --target ${supported_test_suites}
    COMMAND
        ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/generate_ztest_files.py
           ${test_suite_list_file}
           ${tf_psa_crypto_build_tests_path}
    DEPENDS
        ${tf_psa_crypto_test_src_templates}
        ${tf_psa_crypto_test_data_files}
        ${tf_psa_crypto_test_datax_files}
        ${test_suite_list_file}
        ${PROJECT_SOURCE_DIR}/generate_ztest_files.py
  )

  add_custom_target(gen_tests_suite_source
    ALL DEPENDS
      ${generated_test_sources}
      ${PROJECT_SOURCE_DIR}/generate_ztest_files.py
      #${tf_psa_crypto_test_src_templates}
      #${tf_psa_crypto_test_data_files}
  )

  add_dependencies(app gen_tests_suite_source)

  # Add build of the test suite source file
  set(ZTEST_ADDED_SRC
    ${ZTEST_ADDED_SRC} ${test_source}
    PARENT_SCOPE
  )
endfunction()

#set(CONFIG_TESTS_CRYPTO_PSA_TEST_SUITE_LIST
#  ${PROJECT_SOURCE_DIR}/test_suite_list.txt
#)

generate_test_suite_source(
  ${PROJECT_SOURCE_DIR}/test_suite_lists/${CONFIG_PSA_TEST_SUITE_LIST}
)

target_include_directories(app PRIVATE
  include
  # test_suite_*.datax.h are currently genrated in src/
  # src
  ${tf_psa_crypto_build_tests_path}/include
  ${mbdtls_framwork_path}/tests/include
  # PSA Crypto test suite needs some tf-psa-crypto internal header files
  ${CONFIG_TF_PSA_CRYPTO_PATH}/core
  ${CONFIG_TF_PSA_CRYPTO_PATH}/drivers/builtin/src
  ${CONFIG_TF_PSA_CRYPTO_PATH}/drivers/pqcp/src
  ${CONFIG_TF_PSA_CRYPTO_PATH}/drivers/pqcp/include/tf-psa-crypto/private
  ${CONFIG_TF_PSA_CRYPTO_PATH}/platform
  ${CONFIG_TF_PSA_CRYPTO_PATH}/utilities
)

set(mbedtls_framework_sources
  ${mbdtls_framwork_path}/tests/src/asn1_helpers.c
  ${mbdtls_framwork_path}/tests/src/bignum_codepath_check.c
  ${mbdtls_framwork_path}/tests/src/bignum_helpers.c
  #drivers
  #${mbdtls_framwork_path}/tests/src/fake_external_rng_for_test.c
  ${mbdtls_framwork_path}/tests/src/fork_helpers.c
  ${mbdtls_framwork_path}/tests/src/helpers.c
  ${mbdtls_framwork_path}/tests/src/pk_helpers.c
  ${mbdtls_framwork_path}/tests/src/psa_crypto_helpers.c
  ${mbdtls_framwork_path}/tests/src/psa_crypto_stubs.c
  ${mbdtls_framwork_path}/tests/src/psa_exercise_key.c
  ${mbdtls_framwork_path}/tests/src/psa_memory_poisoning_wrappers.c
  #${mbdtls_framwork_path}/tests/src/random.c
  ${mbdtls_framwork_path}/tests/src/test_memory.c
  ${mbdtls_framwork_path}/tests/src/threading_helpers.c
)

set(tf_psa_crypto_sources
  #${CONFIG_TF_PSA_CRYPTO_PATH}/utilities/asn1parse.c
  #${CONFIG_TF_PSA_CRYPTO_PATH}/utilities/asn1write.c
  #${CONFIG_TF_PSA_CRYPTO_PATH}/utilities/base64.c
  #${CONFIG_TF_PSA_CRYPTO_PATH}/utilities/constant_time.c
  #${CONFIG_TF_PSA_CRYPTO_PATH}/utilities/oid.c
  #${CONFIG_TF_PSA_CRYPTO_PATH}/utilities/pem.c
  #${CONFIG_TF_PSA_CRYPTO_PATH}/utilities/pkcs5.c
)

# Build tf-psa-crypto test sources file to a Zephyr application using Ztest
target_compile_definitions(app PRIVATE
  __ZEPHYR__
)

target_sources(app PRIVATE
  ${generated_test_sources}
  ${mbedtls_framework_sources}
  ${tf_psa_crypto_sources}
)
