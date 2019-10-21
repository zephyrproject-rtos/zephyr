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
set(TFMCONFIG "ConfigRegressionIPC")

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

# Find Python 3.x interpreter
find_package(PythonInterp 3)
if (NOT PYTHONINTERP_FOUND)
	message(FATAL_ERROR "Failed to find Python 3.x interpreter. Python3 must be installed and available on the PATH.")
endif()

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
        tfmbuild_exec("${CMAKE_COMMAND} -G\"Unix Makefiles\" -DPROJ_CONFIG=`${READLINK} -f ../configs/${TFMCONFIG}.cmake` -DTFM_PSA_API=True -DTARGET_PLATFORM=${TFMBOARD} -DBL2=True -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCOMPILER=GNUARM ../")
        tfmbuild_exec("make")
        tfmbuild_exec("make install")
endif()

target_include_directories(app PRIVATE ${TFM_PSA_API_DIR}/inc)
target_include_directories(app PRIVATE ${TFM_REMOTE_DIR}/tfm)

target_compile_definitions(app PRIVATE -DTFM_PSA_API)

# target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_audit_api.c)
target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_crypto_api.c)
target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_initial_attestation_api.c)
# target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_nspm_api.c)
target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_nspm_svc_handler.c)
# target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_platform_api.c)
target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_psa_ns_api.c)
target_sources(app PRIVATE ${TFM_PSA_API_DIR}/src/tfm_sst_api.c)

FILE(GLOB app_sources ${APPLICATION_SOURCE_DIR}/src/*.c)
target_sources(app PRIVATE ${app_sources})

target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${TFM_REMOTE_DIR}/tfm/build/install/export/tfm/veneers/s_veneers.o)


# if (NOT DEFINED IMAGE_VERSION)
# 	set(IMAGE_VERSION 0.0.0+0)
# endif()
#
# # Create and sign concatenated binary image, aligned with TF-M BL2
# message(STATUS "Concatenating and signing secure and non-secure images")
# set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
# 	# Create concatenated binary image from the two binary file
# 	COMMAND ${PYTHON_EXECUTABLE} ${TFM_REMOTE_DIR}/tfm/bl2/ext/mcuboot/scripts/assemble.py
# 	ARGS --layout ../../../../build/image_macros_preprocessed.c
# 		 -s ${TFM_REMOTE_DIR}/tfm/build/unit_test/tfm_s.bin
# 		 -n ${CMAKE_BINARY_DIR}/zephyr/zephyr.bin
# 		 -o ${CMAKE_BINARY_DIR}/tfm_full.bin
#
# 	# Sign concatenated binary image with public key in mcuboot folder
# 	COMMAND ${PYTHON_EXECUTABLE} ${TFM_REMOTE_DIR}/tfm/bl2/ext/mcuboot/scripts/imgtool.py
# 	ARGS sign
# 		 --layout ../../../../build/image_macros_preprocessed.c
# 		 -k ../../../../bl2/ext/mcuboot/root-rsa-3072.pem
# 		 --align 1
# 		 -v ${IMAGE_VERSION}
# 		 ${ADD_SECURITY_COUNTER}
# 		 -H 0x400
# 		 ${CMAKE_BINARY_DIR}/tfm_full.bin
# 		 ${CMAKE_BINARY_DIR}/tfm_sign.bin
#
# 	# Copy mcuboot.bin
# 	COMMAND cp ${TFM_REMOTE_DIR}/tfm/build/bl2/ext/mcuboot/mcuboot.bin ${CMAKE_BINARY_DIR})
