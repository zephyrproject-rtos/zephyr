# Copyright (c) 2019 Linaro
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

#set(ZEPHYRBOARD "v2m_musca")
#set(TFMBOARD "MUSCA_A")
set(ZEPHYRBOARD "mps2_an521_nonsecure")
set(TFMBOARD "AN521")

# Make sure we are using a compatible board target.
if(NOT (${BOARD} STREQUAL ${ZEPHYRBOARD}))
        message(FATAL_ERROR "${BOARD} unsupported. Please use \'${ZEPHYRBOARD}\'.")
endif()

# Select a known trusted-firmware-m commit
set(TFM_COMMIT_HASH "0d822496e809132e040c0a5cd556b2d5dad01ea6")

# Set dependencies for the above commit
set(TFM_GIT_TFM "${GIT_EXECUTABLE} clone https://git.trustedfirmware.org/trusted-firmware-m.git tfm")
set(TFM_GIT_TLS "${GIT_EXECUTABLE} clone https://github.com/ARMmbed/mbedtls.git -b mbedtls-2.7.9")
set(TFM_GIT_CRYPTO "${GIT_EXECUTABLE} clone https://github.com/ARMmbed/mbed-crypto.git -b mbedcrypto-1.0.0")
set(TFM_GIT_CMSIS "${GIT_EXECUTABLE} clone https://github.com/ARM-software/CMSIS_5.git -b 5.2.0")

# Set the TF-M output folders. This is placed here and not in the build folder
# since you will generally only want to do the clone and build operations
# once for trusted-firmware-m and dependencies.
set(TFM_REMOTE_DIR "$ENV{ZEPHYR_BASE}/ext/tfm")
set(TFM_PSA_API_DIR "${TFM_REMOTE_DIR}/tfm/build/install/export/tfm")

# Assign the full git path to ${GIT_EXECUTABLE}.
find_package(Git)

# Set an ARM toolchain path for the CI docker image
set(TFM_CI_ARM "export PATH=$PATH:/opt/toolchain/arm-none-eabi/bin")

# Run the specified git command string from ${TFM_REMOTE_DIR}.
function(remote_exec cmd)
        message(STATUS "Executing \'${cmd}\'")
        # TODO: Fix this since it will only work with bash!
        execute_process(
                COMMAND bash -c "${cmd}"
                WORKING_DIRECTORY ${TFM_REMOTE_DIR}
                OUTPUT_VARIABLE PROCESS_OUTPUT
                OUTPUT_STRIP_TRAILING_WHITESPACE)
        message("${PROCESS_OUTPUT}")
endfunction()

# Run the specified bash command string from ${TFM_REMOTE_DIR}/tfm/build.
function(tfmbuild_exec cmd)
        message(STATUS "Executing \'${cmd}\'")
        # TODO: Fix this since it will only work with bash!
        execute_process(
                # TODO: Only prepend path in CI environment
                COMMAND bash -c "${TFM_CI_ARM}; ${cmd}"
                WORKING_DIRECTORY ${TFM_REMOTE_DIR}/tfm/build
                OUTPUT_VARIABLE PROCESS_OUTPUT
                OUTPUT_STRIP_TRAILING_WHITESPACE)
        message("${PROCESS_OUTPUT}")
endfunction()

# Check if trusted-firmware-m build folder already exists.
if(EXISTS ${TFM_REMOTE_DIR}/tfm/build/install)
        set(TFM_BUILD_EXISTS "TRUE")
        message(STATUS "TF-M build/install folder already exists: \'${TFM_REMOTE_DIR}/tfm/build/install/\'")
endif()

# Clone tf-m repo and dependencies if necessary
if(NOT TFM_BUILD_EXISTS)
        # Clone required repos.
        message(STATUS "Cloning required repos for trusted-firmware-m")
        file(REMOVE_RECURSE ${TFM_REMOTE_DIR})
        file(MAKE_DIRECTORY ${TFM_REMOTE_DIR})
        remote_exec(${TFM_GIT_TLS})
        remote_exec(${TFM_GIT_CRYPTO})
        remote_exec(${TFM_GIT_CMSIS})
        remote_exec(${TFM_GIT_TFM})

        # Start tf-m build process:
        message(STATUS "Building trusted-firmware-m secure library")
        set(READLINK readlink)
        find_program(GREADLINK greadlink)
        if(GREADLINK)
                # Check for 'greadlink' on OS X ('brew install coreutils').
                set(READLINK greadlink)
        endif()
        file(MAKE_DIRECTORY ${TFM_REMOTE_DIR}/tfm/build)
        tfmbuild_exec("cd .. && ${GIT_EXECUTABLE} reset --hard ${TFM_COMMIT_HASH}")
        tfmbuild_exec("${CMAKE_COMMAND} -G\"Unix Makefiles\" -DPROJ_CONFIG=`${READLINK} -f ../configs/ConfigRegressionIPC.cmake` -DTFM_PSA_API=True -DTARGET_PLATFORM=${TFMBOARD} -DBL2=True -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCOMPILER=GNUARM ../")
        tfmbuild_exec("make")
        tfmbuild_exec("make install")
endif()

target_include_directories(app PRIVATE ${TFM_PSA_API_DIR}/inc)
target_include_directories(app PRIVATE ${TFM_REMOTE_DIR}/tfm)
target_include_directories(app PRIVATE ${TFM_REMOTE_DIR}/tfm/test/suites/crypto/)
target_include_directories(app PRIVATE ${TFM_REMOTE_DIR}/tfm/test/framework/)

target_compile_definitions(app PRIVATE -DTFM_PSA_API)
target_compile_definitions(app PRIVATE -DDOMAIN_NS=1)

# target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_audit_api.c)
target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_crypto_api.c)
target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_initial_attestation_api.c)
# target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_nspm_api.c)
target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_nspm_svc_handler.c)
# target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_platform_api.c)
target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_psa_ns_api.c)
target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_sst_api.c)
target_sources(app PRIVATE ${TFM_REMOTE_DIR}/tfm/test/framework/test_framework.c)
target_sources(app PRIVATE ${TFM_REMOTE_DIR}/tfm/test/framework/test_framework_helpers.c)
target_sources(app PRIVATE ${TFM_REMOTE_DIR}/tfm/test/framework/test_framework_integ_test_helper.c)
target_sources(app PRIVATE ${TFM_REMOTE_DIR}/tfm/test/suites/crypto/crypto_tests_common.c)
target_sources(app PRIVATE ${TFM_REMOTE_DIR}/tfm/test/suites/crypto/non_secure/crypto_ns_interface_testsuite.c)

FILE(GLOB app_sources ${APPLICATION_SOURCE_DIR}/src/*.c)
target_sources(app PRIVATE ${app_sources})

target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${TFM_REMOTE_DIR}/tfm/build/install/export/tfm/veneers/s_veneers.o)

# Add post-build merge step
# add_custom_command(
#         TARGET app POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E echo app_post_build
#         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#         COMMENT "Merging ${ZEPHYR_BINARY_DIR}/zephyr.hex with secure TF-M lib for board: ${BOARD}"
# )
