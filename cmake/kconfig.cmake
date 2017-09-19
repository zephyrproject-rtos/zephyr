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

set(kconfig_target_list
  config
  gconfig
  menuconfig
  oldconfig
  xconfig
  )

set(COMMAND_FOR_config     ${KCONFIG_CONF} --oldaskconfig ${PROJECT_SOURCE_DIR}/Kconfig)
set(COMMAND_FOR_gconfig    gconf                          ${PROJECT_SOURCE_DIR}/Kconfig)
set(COMMAND_FOR_menuconfig ${KCONFIG_MCONF}               ${PROJECT_SOURCE_DIR}/Kconfig)
set(COMMAND_FOR_oldconfig  ${KCONFIG_CONF} --oldconfig    ${PROJECT_SOURCE_DIR}/Kconfig)
set(COMMAND_FOR_xconfig    qconf                          ${PROJECT_SOURCE_DIR}/Kconfig)

foreach(kconfig_target ${kconfig_target_list})
  add_custom_target(
    ${kconfig_target}
    ${CMAKE_COMMAND} -E env
    srctree=${PROJECT_SOURCE_DIR}
    KERNELVERSION=${PROJECT_VERSION}
    KCONFIG_CONFIG=${DOTCONFIG}
    ${COMMAND_FOR_${kconfig_target}}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/kconfig
    )
endforeach()

if(NOT EXISTS ${DOTCONFIG})
  set(
    merge_config_files
    ${BOARD_DEFCONFIG}
    ${OVERLAY_CONFIG}
    ${CONF_FILE_AS_LIST}
    )
  foreach(merge_config_input ${merge_config_files})
    if(IS_ABSOLUTE ${merge_config_input})
      set(path ${merge_config_input})
    else()
      set(path ${APPLICATION_SOURCE_DIR}/${merge_config_input})
    endif()

    if(NOT EXISTS ${path})
      message(FATAL_ERROR "File not found: ${path}")
    endif()
  endforeach()

  execute_process(
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/scripts/kconfig/merge_config.py
    -m
    -q
    -O ${PROJECT_BINARY_DIR}
    ${merge_config_files}
    WORKING_DIRECTORY ${APPLICATION_SOURCE_DIR}
    # The working directory is set to the app dir such that the user
    # can use relative paths in CONF_FILE, e.g. CONF_FILE=nrf5.conf
    RESULT_VARIABLE ret
  )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()

  execute_process(
	  COMMAND ${KCONFIG_CONF}
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
foreach(merge_config_input ${merge_config_files} ${DOTCONFIG})
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${merge_config_input})
endforeach()

message(STATUS "Generating zephyr/include/generated/autoconf.h")
execute_process(
	COMMAND ${KCONFIG_CONF}
    --silentoldconfig
    ${PROJECT_SOURCE_DIR}/Kconfig
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/kconfig
)

add_custom_target(config-sanitycheck DEPENDS ${DOTCONFIG})

# Parse the lines prefixed with CONFIG_ in the .config file from Kconfig
import_kconfig(${DOTCONFIG})
