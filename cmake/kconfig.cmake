file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include/generated)
# Folders needed for conf/mconf files (kconfig has no method of redirecting all output files).
# conf/mconf needs to be run from a different directory because of: ZEP-1963
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/kconfig/include/generated)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/kconfig/include/config)

set(BOARD_DEFCONFIG ${PROJECT_SOURCE_DIR}/boards/${ARCH}/${BOARD}/${BOARD}_defconfig)
set(DOTCONFIG       ${PROJECT_BINARY_DIR}/.config)

if(CONF_FILE)
  # CONF_FILE has been specified on the cmake CLI and has precedence
elseif(DEFINED ENV{CONF_FILE})
  set(CONF_FILE $ENV{CONF_FILE})
elseif(EXISTS   ${APPLICATION_SOURCE_DIR}/prj.conf)
  set(CONF_FILE ${APPLICATION_SOURCE_DIR}/prj.conf)
endif()

set(CONF_FILE ${CONF_FILE} CACHE STRING "If desired, you can build the application using\
the configuration settings specified in an alternate .conf file using this parameter. \
These settings will override the settings in the application’s .config file or its default .conf file. ")

set(ENV{srctree}            ${PROJECT_SOURCE_DIR})
set(ENV{KERNELVERSION}      ${PROJECT_VERSION})
set(ENV{KCONFIG_CONFIG}     ${DOTCONFIG})
set(ENV{KCONFIG_AUTOHEADER} ${AUTOCONF_H})

# Create new .config if the file does not exists, or the user has
# edited one of the configuration files.
set(CREATE_NEW_DOTCONFIG "")
if(NOT EXISTS ${DOTCONFIG} OR ${BOARD_DEFCONFIG} IS_NEWER_THAN ${DOTCONFIG})
  set(CREATE_NEW_DOTCONFIG 1)
elseif(${CONF_FILE})
  if(${CONF_FILE} IS_NEWER_THAN ${DOTCONFIG})
    set(CREATE_NEW_DOTCONFIG 1)
  endif()
endif()
if(CREATE_NEW_DOTCONFIG)
  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/kconfig/merge_config.py -m -q
      -O ${PROJECT_BINARY_DIR}
      ${BOARD_DEFCONFIG} ${CONF_FILE}
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
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CONF_FILE})
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${DOTCONFIG})

message(STATUS "Generating zephyr/include/generated/autoconf.h")
execute_process(
  COMMAND ${PREBUILT_HOST_TOOLS}/kconfig/conf
    --silentoldconfig
    ${PROJECT_SOURCE_DIR}/Kconfig
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/kconfig
)

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
