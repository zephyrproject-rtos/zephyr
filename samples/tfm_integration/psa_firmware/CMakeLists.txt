# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tfm_psa_firmware)


if (NOT CONFIG_APP_FIRMWARE_UPDATE_IMAGE)
   message(FATAL_ERROR "CONFIG_APP_FIRMWARE_UPDATE_IMAGE required")
endif()

# NOTE: These must not include ${CMAKE_BINARY_DIR} otherwise you get an
# absolute path as part of the C symbols in the generated object file.
# This is difficult to use in a correct and portable way. Instead, we will
# take advantage of running everything from within the build directory.
set(UPDATE_SIGNED_HEX update-signed.hex)
set(UPDATE_BIN update-image.bin)
set(UPDATE_OBJ update-image.o)
set(UPDATE_HEADER_BIN update-header.bin)
set(UPDATE_HEADER_OBJ update-header.o)

# The following sequence of add_custom_command calls builds a dependency
# graph of all the bits we need to sign# an image. The process looks
# something like:
#
#                 [(1) sample.hex ]
#                         |
#                         v
#              [(2) sign with imgtool ]
#                         |
#                         v
#               [(3) split-header.py ]
#                  |             |
#              app |             | header
#                  v             v
#             [(4,5) objdump bin to obj ]
#                  |             |
#          app obj |             | header obj
#                  v             v
#              [(6) target_sources(..) ]
#
# Note that node (1) is an input.

# This is duplicated from the trusted-firmware-m CMakeLists.txt, as this
# needs it and CMAKE does not allow us to import the varibales from that
# directory.
set(TFM_MCUBOOT_DIR "${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/bl2/ext/mcuboot")
# Node (2) in the above graphic
add_custom_command(
  DEPENDS ${CONFIG_APP_FIRMWARE_UPDATE_IMAGE}
  OUTPUT ${UPDATE_SIGNED_HEX}
  COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${ZEPHYR_MCUBOOT_MODULE_DIR}/scripts
    ${PYTHON_EXECUTABLE}
    ${TFM_MCUBOOT_DIR}/scripts/wrapper/wrapper.py
    --layout "${CMAKE_BINARY_DIR}/tfm/bl2/ext/mcuboot/CMakeFiles/signing_layout_ns.dir/signing_layout_ns.o"
    -k ${CONFIG_TFM_KEY_FILE_NS}
    --public-key-format "full"
    --align 1
    -v ${CONFIG_APP_FIRMWARE_UPDATE_IMAGE_VERSION}
    --pad
    -s auto
    -H ${CONFIG_ROM_START_OFFSET}
    ${CONFIG_APP_FIRMWARE_UPDATE_IMAGE}
    ${UPDATE_SIGNED_HEX}
)

# Node (3) in the above graphic
add_custom_command(
  OUTPUT ${UPDATE_HEADER_BIN}
  OUTPUT ${UPDATE_BIN}
  DEPENDS ${UPDATE_SIGNED_HEX}
  COMMAND ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_LIST_DIR}/split-header.py
    ${UPDATE_SIGNED_HEX}
    ${UPDATE_BIN}
    ${UPDATE_HEADER_BIN}
)

# Node (4) in the above graphic
add_custom_command(
  OUTPUT ${UPDATE_HEADER_OBJ}
  DEPENDS ${UPDATE_HEADER_BIN}
  COMMAND ${CMAKE_OBJCOPY} -I binary -O elf32-littlearm -B arm
    ${UPDATE_HEADER_BIN}
    ${UPDATE_HEADER_OBJ}
)

# Node (5) in the above graphic
add_custom_command(
  OUTPUT ${UPDATE_OBJ}
  DEPENDS ${UPDATE_BIN}
  COMMAND ${CMAKE_OBJCOPY} -I binary -O elf32-littlearm -B arm
    ${UPDATE_BIN}
    ${UPDATE_OBJ}
)

# Source files in this sample
# Node (6) in the above graphic
target_sources(app PRIVATE src/main.c ${UPDATE_OBJ} ${UPDATE_HEADER_OBJ})

target_include_directories(app PRIVATE
  $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/install/interface/include
)
