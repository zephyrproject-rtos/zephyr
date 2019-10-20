# Copyright (c) 2019 Linaro
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
project(NONE)

# Make sure we are using a compatible board target.
if((${BOARD} STREQUAL "mps2_an521_nonsecure"))
	set(TFMBOARD "AN521")
else()
        message(FATAL_ERROR "${BOARD} unsupported. Please use \'${ZEPHYRBOARD}\'.")
endif()

if(NOT DEFINED TFMCONFIG)
	set(TFMCONFIG "ConfigRegressionIPC")
endif()

# Select a known trusted-firmware-m commit
set(TFM_COMMIT_HASH "5358c8c4cf385cde6091adc74dce53fa3f0c2ce4")

# Set dependencies for the above commit
set(TFM_GIT_TFM "${GIT_EXECUTABLE} clone https://git.trustedfirmware.org/trusted-firmware-m.git tfm")
set(TFM_GIT_TLS "${GIT_EXECUTABLE} clone https://github.com/ARMmbed/mbedtls.git -b mbedtls-2.7.9")
set(TFM_GIT_CRYPTO "${GIT_EXECUTABLE} clone https://github.com/ARMmbed/mbed-crypto.git -b mbedcrypto-1.1.0")
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

# Clone TF-M repo and dependencies if necessary
if(NOT TFM_BUILD_EXISTS)
        # Clone required repos.
        message(STATUS "Cloning required repos for trusted-firmware-m")
        file(REMOVE_RECURSE ${TFM_REMOTE_DIR}/CMSIS_5)
        file(REMOVE_RECURSE ${TFM_REMOTE_DIR}/mbed-crypto)
        file(REMOVE_RECURSE ${TFM_REMOTE_DIR}/mbedtls)
        file(REMOVE_RECURSE ${TFM_REMOTE_DIR}/tfm)
        remote_exec(${TFM_GIT_TLS})
        remote_exec(${TFM_GIT_CRYPTO})
        remote_exec(${TFM_GIT_CMSIS})
        remote_exec(${TFM_GIT_TFM})

        # Start TF-M build process:
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
        tfmbuild_exec("cmake --build . -- install")
endif()
