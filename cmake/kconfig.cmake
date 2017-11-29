# Folders needed for conf/mconf files (kconfig has no method of redirecting all output files).
# conf/mconf needs to be run from a different directory because of: ZEP-1963
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/kconfig/include/generated)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/kconfig/include/config)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include/generated)

set_ifndef(KCONFIG_ROOT ${PROJECT_SOURCE_DIR}/Kconfig)

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

set(COMMAND_FOR_config     ${KCONFIG_CONF} --oldaskconfig ${KCONFIG_ROOT})
set(COMMAND_FOR_gconfig    gconf                          ${KCONFIG_ROOT})
set(COMMAND_FOR_menuconfig ${KCONFIG_MCONF}               ${KCONFIG_ROOT})
set(COMMAND_FOR_oldconfig  ${KCONFIG_CONF} --oldconfig    ${KCONFIG_ROOT})
set(COMMAND_FOR_xconfig    qconf                          ${KCONFIG_ROOT})

foreach(kconfig_target ${kconfig_target_list})
  add_custom_target(
    ${kconfig_target}
    ${CMAKE_COMMAND} -E env
    srctree=${PROJECT_SOURCE_DIR}
    KERNELVERSION=${PROJECT_VERSION}
    KCONFIG_CONFIG=${DOTCONFIG}
    ${COMMAND_FOR_${kconfig_target}}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/kconfig
    USES_TERMINAL
    )
endforeach()

# Bring in extra configuration files dropped in by the user or anyone else;
# make sure they are set at the end so we can override any other setting
file(GLOB config_files ${APPLICATION_BINARY_DIR}/*.conf)
list(SORT config_files)
set(
  merge_config_files
  ${BOARD_DEFCONFIG}
  ${CONF_FILE_AS_LIST}
  ${OVERLAY_CONFIG}
  ${config_files}
)

# Create a list of absolute paths to the .config sources from
# merge_config_files, which is a mix of absolute and relative paths.
set(merge_config_files_with_absolute_paths "")
foreach(f ${merge_config_files})
  if(IS_ABSOLUTE ${f})
    set(path ${f})
  else()
    set(path ${APPLICATION_SOURCE_DIR}/${f})
  endif()

  list(APPEND merge_config_files_with_absolute_paths ${path})
endforeach()

foreach(f ${merge_config_files_with_absolute_paths})
  if(NOT EXISTS ${f} OR IS_DIRECTORY ${f})
    message(FATAL_ERROR "File not found: ${f}")
  endif()
endforeach()

# Calculate a checksum of merge_config_files to determine if we need
# to re-generate .config
set(merge_config_files_checksum "")
foreach(f ${merge_config_files_with_absolute_paths})
  file(MD5 ${f} checksum)
  set(merge_config_files_checksum "${merge_config_files_checksum}${checksum}")
endforeach()

# Create a new .config if it does not exists, or if the checksum of
# the dependencies has changed
set(merge_config_files_checksum_file ${PROJECT_BINARY_DIR}/.cmake.dotconfig.checksum)
set(CREATE_NEW_DOTCONFIG "")
if(NOT EXISTS ${DOTCONFIG})
  set(CREATE_NEW_DOTCONFIG 1)
else()
  # Read out what the checksum was previously
  file(READ
    ${merge_config_files_checksum_file}
    merge_config_files_checksum_prev
    )
  set(CREATE_NEW_DOTCONFIG 1)
  if(
      ${merge_config_files_checksum} STREQUAL
      ${merge_config_files_checksum_prev}
      )
    set(CREATE_NEW_DOTCONFIG 0)
  endif()
endif()
if(CREATE_NEW_DOTCONFIG)
  execute_process(
    COMMAND
    ${PROJECT_SOURCE_DIR}/scripts/kconfig/merge_config.sh
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
      ${KCONFIG_ROOT}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/kconfig
    RESULT_VARIABLE ret
  )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()

  file(WRITE
    ${merge_config_files_checksum_file}
    ${merge_config_files_checksum}
    )
endif()

# Force CMAKE configure when the configuration files changes.
foreach(merge_config_input ${merge_config_files} ${DOTCONFIG})
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${merge_config_input})
endforeach()

message(STATUS "Generating zephyr/include/generated/autoconf.h")
execute_process(
	COMMAND ${KCONFIG_CONF}
    --silentoldconfig
    ${KCONFIG_ROOT}
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/kconfig
)

add_custom_target(config-sanitycheck DEPENDS ${DOTCONFIG})

# Parse the lines prefixed with CONFIG_ in the .config file from Kconfig
import_kconfig(${DOTCONFIG})
