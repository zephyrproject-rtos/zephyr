# Intel ADSP SoCs family CMake file
#
# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0


find_program(RIMAGE NAMES rimage)

if (DEFINED RIMAGE)
message(STATUS "Found rimage ${RIMAGE}")
else()
message(STATUS "No rimage found")
endif()

# The path to where toml configurations are by default
# will be the rimage config directory in the sof module
# but may be overridden with an environment variable if
# needed.
if (DEFINED ENV{RIMAGE_CONF_PATH}
    AND EXISTS "$ENV{RIMAGE_CONF_PATH}")
set(RIMAGE_CONF_PATH "$ENV{RIMAGE_CONF_PATH}")
else()
# TODO this *will* change once rimage is a west only module
set(RIMAGE_CONF_PATH "${ZEPHYR_SOF_MODULE_DIR}/rimage/config")
endif()

# The path to where signing keys are by default will be the sof module but may
# be overridden with an environment variable if needed.
if (DEFINED ENV{RIMAGE_KEY_PATH}
    AND EXISTS "$ENV{RIMAGE_KEY_PATH}")
set(RIMAGE_KEY_PATH "$ENV{RIMAGE_KEY_PATH}")
else()
set(RIMAGE_KEY_PATH "${ZEPHYR_SOF_MODULE_DIR}/keys")
endif()

# A direct env var RIMAGE_KEY_FILE may be given, otherwise
# the board may define the RIMAGE_SIGN_KEY name along with an env
# var to define the path to where key files are stored.
if (DEFINED ENV{RIMAGE_KEY_FILE}
    AND EXISTS "$ENV{RIMAGE_KEY_FILE}")
set(RIMAGE_KEY_FILE ENV{RIMAGE_KEY_FILE})
message(STATUS "Using rimage key file from ENV{RIMAGE_KEY_FILE} ${RIMAGE_KEY_FILE}")
elseif(DEFINED RIMAGE_KEY_PATH
    AND DEFINED RIMAGE_SIGN_KEY
    AND EXISTS "${RIMAGE_KEY_PATH}/${RIMAGE_SIGN_KEY}")
set(RIMAGE_KEY_FILE "${RIMAGE_KEY_PATH}/${RIMAGE_SIGN_KEY}")
message(STATUS "Using rimage key file from RIMAGE_KEY_PATH/RIMAGE_SIGN_KEY ${RIMAGE_KEY_FILE}")
elseif(DEFINED RIMAGE_SIGN_KEY)
set(RIMAGE_KEY_FILE "")
message(STATUS "No rimage key file defined, RIMAGE_SIGN_KEY ${RIMAGE_SIGN_KEY}, RIMAGE_KEY_PATH $ENV{RIMAGE_KEY_PATH}, RIMAGE_KEY_FILE $ENV{RIMAGE_KEY_FILE}")
endif()

# A direct env var RIMAGE_CONF_FILE may be given, otherwise
# the board may define the RIMAGE_TARGET name along with an env
# var to define the path to where config files are stored.
if (DEFINED ENV{RIMAGE_CONF_FILE}
    AND EXISTS "$ENV{RIMAGE_CONF_FILE}")
set(RIMAGE_CONF_FILE ENV{RIMAGE_CONF_FILE})
message(STATUS "Using rimage conf file from ENV{RIMAGE_CONF_FILE} ${RIMAGE_CONF_FILE}")
elseif (DEFINED RIMAGE_CONF_PATH
    AND DEFINED RIMAGE_TARGET
    AND EXISTS "${RIMAGE_CONF_PATH}/${RIMAGE_TARGET}.toml")
set(RIMAGE_CONF_FILE "${RIMAGE_CONF_PATH}/${RIMAGE_TARGET}.toml")
message(STATUS "Using rimage conf file from RIMAGE_CONF_PATH/RIMAGE_TARGET.toml ${RIMAGE_CONF_FILE}")
else()
message(STATUS "No rimage config file defined, RIMAGE_TARGET ${RIMAGE_TARGET}, RIMAGE_CONF_PATH $ENV{RIMAGE_CONF_PATH}, RIMAGE_CONF_FILE $ENV{RIMAGE_CONF_FILE}")
endif()

# If rimage is in the path, and a valid key and conf file path are
# defined, then add an rimage target (signed image target)
if(DEFINED RIMAGE
    AND DEFINED RIMAGE_KEY_FILE
    AND DEFINED RIMAGE_CONF_FILE)
message(STATUS "Intel ADSP adding signed image target with rimage")
add_custom_target(rimage_target ALL
  DEPENDS ${CMAKE_BINARY_DIR}/zephyr/boot.mod ${CMAKE_BINARY_DIR}/zephyr/main.mod
  COMMAND ${RIMAGE}
  	-k ${RIMAGE_KEY_FILE}
    -c ${RIMAGE_CONF_FILE}
    -o ${CMAKE_BINARY_DIR}/zephyr/zephyr.ri
    ${CMAKE_BINARY_DIR}/zephyr/boot.mod
    ${CMAKE_BINARY_DIR}/zephyr/main.mod)
else()
message(STATUS "Missing rimage or an appropriate environment variable for finding conf and key files, rimage will not be run")
endif()

add_subdirectory(common)
if(CONFIG_SOC_SERIES_INTEL_ACE)
	add_subdirectory(ace)
endif()
if(CONFIG_INTEL_ADSP_CAVS)
	add_subdirectory(cavs)
endif()
