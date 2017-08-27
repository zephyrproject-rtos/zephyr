file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include/generated)
# Folders needed for conf/mconf files (kconfig has no method of redirecting all output files).
# conf/mconf needs to be run from a different directory because of: ZEP-1963
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/kconfig/include/generated)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/kconfig/include/config)

#set(BOARD_DEFCONFIG ${PROJECT_SOURCE_DIR}/boards/${ARCH}/${BOARD}/${BOARD}_defconfig)
set(BOARD_DEFCONFIG ${BOARD_DIR}/${BOARD}_defconfig)
set(DOTCONFIG       ${PROJECT_BINARY_DIR}/.config)

if(CONF_FILE)
string(REPLACE " " ";" CONF_FILE_AS_LIST ${CONF_FILE})
endif()


set(ENV{srctree}            ${PROJECT_SOURCE_DIR})
set(ENV{KERNELVERSION}      ${PROJECT_VERSION})
set(ENV{KCONFIG_CONFIG}     ${DOTCONFIG})
set(ENV{KCONFIG_AUTOHEADER} ${AUTOCONF_H})

if(IS_TEST)
  list(APPEND OVERLAY_CONFIG $ENV{ZEPHYR_BASE}/tests/include/test.config)
endif()

# Create new .config if the file does not exists, or the user has
# edited one of the configuration files.
set(CREATE_NEW_DOTCONFIG "")
if(NOT EXISTS ${DOTCONFIG} OR ${BOARD_DEFCONFIG} IS_NEWER_THAN ${DOTCONFIG})
  set(CREATE_NEW_DOTCONFIG 1)
elseif(CONF_FILE)
  foreach(CONF_FILE_ITEM ${CONF_FILE_AS_LIST})
    if(${CONF_FILE_ITEM} IS_NEWER_THAN ${DOTCONFIG})
      set(CREATE_NEW_DOTCONFIG 1)
    endif()
  endforeach()
elseif(NOT CONF_FILE AND NOT EXISTS ${DOTCONFIG})
      set(CREATE_NEW_DOTCONFIG 1)
endif()
if(CREATE_NEW_DOTCONFIG)
  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/kconfig/merge_config.py -m -q
      -O ${PROJECT_BINARY_DIR}
      ${BOARD_DEFCONFIG}
      ${OVERLAY_CONFIG}
      ${CONF_FILE_AS_LIST}
    WORKING_DIRECTORY ${APPLICATION_SOURCE_DIR}
    # The working directory is set to the app dir such that the user
    # can use relative paths in CONF_FILE, e.g. CONF_FILE=nrf5.conf
    RESULT_VARIABLE ret
  )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()

  execute_process(
    COMMAND ${PREBUILT_HOST_TOOLS}/kconfig/conf
      --olddefconfig
      ${PROJECT_SOURCE_DIR}/Kconfig
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/kconfig
    RESULT_VARIABLE ret
  )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()

endif()

# Force CMAKE configure when the configuration files changes.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${BOARD_DEFCONFIG})
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${DOTCONFIG})
foreach(CONF_FILE_ITEM ${CONF_FILE_AS_LIST})
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CONF_FILE_ITEM})
endforeach()

message(STATUS "Generating zephyr/include/generated/autoconf.h")
execute_process(
  COMMAND ${PREBUILT_HOST_TOOLS}/kconfig/conf
    --silentoldconfig
    ${PROJECT_SOURCE_DIR}/Kconfig
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/kconfig
)

add_custom_target(config-sanitycheck DEPENDS ${DOTCONFIG})

add_custom_target(menuconfig
  COMMAND
    ${CMAKE_COMMAND} -E env
      srctree=${PROJECT_SOURCE_DIR}
      KERNELVERSION=${PROJECT_VERSION}
      KCONFIG_CONFIG=${DOTCONFIG}
        ${PREBUILT_HOST_TOOLS}/kconfig/mconf ${PROJECT_SOURCE_DIR}/Kconfig
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/kconfig
  USES_TERMINAL
)

# Parse the lines prefixed with CONFIG_ in the .config file from Kconfig
import_kconfig(${DOTCONFIG})
