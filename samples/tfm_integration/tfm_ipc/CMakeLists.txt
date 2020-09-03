# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

# Override the binary used by qemu
set(QEMU_KERNEL_OPTION "-device;loader,file=${CMAKE_BINARY_DIR}/tfm_qemu.hex")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if (CONFIG_TFM_BL2_TRUE)
   set(TFM_BL2_ARGUMENT BL2 True)
elseif (CONFIG_TFM_BL2_FALSE)
   set(TFM_BL2_ARGUMENT BL2 False)
endif()

# Add "tfm" as an external project via the TF-M module's cmake file
if(${TFM_TARGET_PLATFORM} STREQUAL "STM_NUCLEO_L552ZE_Q")
   trusted_firmware_build(BINARY_DIR ${CMAKE_BINARY_DIR}/tfm
                          BOARD ${TFM_TARGET_PLATFORM}
                          IPC
                          CFGFILE "ConfigRegressionIPCTfmLevel2"
                          OUT_VENEERS_FILE VENEERS_FILE
                          ${TFM_BL2_ARGUMENT}
   )
else()
   trusted_firmware_build(BINARY_DIR ${CMAKE_BINARY_DIR}/tfm
                          BOARD ${TFM_TARGET_PLATFORM}
                          IPC
                          CFGFILE "ConfigRegressionIPC"
                          OUT_VENEERS_FILE VENEERS_FILE
                          ${TFM_BL2_ARGUMENT}
  )
endif()

project(tfm_ipc)

# Set "tfm" as a dependency
add_dependencies(app tfm)

target_sources(app PRIVATE src/main.c)

# Include TF-M secure service source files
target_link_libraries(app PRIVATE tfm_ipc_psa_api)

# Link in veneer function locations
target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${VENEERS_FILE})
