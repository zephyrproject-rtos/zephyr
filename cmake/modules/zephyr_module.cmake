# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

include(extensions)
include(python)

# This cmake file provides functionality to import CMakeLists.txt and Kconfig
# files for Zephyr modules into Zephyr build system.
#
# CMakeLists.txt and Kconfig files can reside directly in the Zephyr module or
# in a MODULE_EXT_ROOT.
# The `<module>/zephyr/module.yml` file specifies whether the build files are
# located in the Zephyr module or in a MODULE_EXT_ROOT.
#
# A list of Zephyr modules can be provided to the build system using:
#   -DZEPHYR_MODULES=<module-path>[;<additional-module(s)-path>]
#
# It looks for: <module>/zephyr/module.yml or
#               <module>/zephyr/CMakeLists.txt
# to load the Zephyr module into Zephyr build system.
# If west is installed, it uses west's APIs to obtain a list of projects to
# search for zephyr/module.yml from the current workspace's manifest.
#
# If the module.yml file specifies that build files are located in a
# MODULE_EXT_ROOT then the variables:
# - `ZEPHYR_<MODULE_NAME>_CMAKE_DIR` is used for inclusion of the CMakeLists.txt
# - `ZEPHYR_<MODULE_NAME>_KCONFIG` is used for inclusion of the Kconfig
# files into the build system.

# Settings used by Zephyr module but where systems may define an alternative value.
set_ifndef(KCONFIG_BINARY_DIR ${CMAKE_BINARY_DIR}/Kconfig)

zephyr_get(ZEPHYR_MODULES)
if(ZEPHYR_MODULES)
  set(ZEPHYR_MODULES_ARG "--modules" ${ZEPHYR_MODULES})
endif()

zephyr_get(ZEPHYR_EXTRA_MODULES)
if(ZEPHYR_EXTRA_MODULES)
  set(ZEPHYR_EXTRA_MODULES_ARG "--extra-modules" ${ZEPHYR_EXTRA_MODULES})
endif()

file(MAKE_DIRECTORY ${KCONFIG_BINARY_DIR})
set(kconfig_modules_file ${KCONFIG_BINARY_DIR}/Kconfig.modules)
set(kconfig_sysbuild_file ${KCONFIG_BINARY_DIR}/Kconfig.sysbuild.modules)
set(cmake_modules_file ${CMAKE_BINARY_DIR}/zephyr_modules.txt)
set(cmake_sysbuild_file ${CMAKE_BINARY_DIR}/sysbuild_modules.txt)
set(zephyr_settings_file ${CMAKE_BINARY_DIR}/zephyr_settings.txt)

if(WEST)
  set(west_arg "--zephyr-base" ${ZEPHYR_BASE})
endif()

if(WEST OR ZEPHYR_MODULES)
  # Zephyr module uses west, so only call it if west is installed or
  # ZEPHYR_MODULES was provided as argument to CMake.
  execute_process(
    COMMAND
    ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/zephyr_module.py
    ${west_arg}
    ${ZEPHYR_MODULES_ARG}
    ${ZEPHYR_EXTRA_MODULES_ARG}
    --kconfig-out ${kconfig_modules_file}
    --cmake-out ${cmake_modules_file}
    --sysbuild-kconfig-out ${kconfig_sysbuild_file}
    --sysbuild-cmake-out ${cmake_sysbuild_file}
    --settings-out ${zephyr_settings_file}
    WORKING_DIRECTORY ${ZEPHYR_BASE}
    ERROR_VARIABLE
    zephyr_module_error_text
    RESULT_VARIABLE
    zephyr_module_return
  )

  if(${zephyr_module_return})
      message(FATAL_ERROR "${zephyr_module_error_text}")
  endif()

  if(EXISTS ${zephyr_settings_file})
    file(STRINGS ${zephyr_settings_file} zephyr_settings_txt ENCODING UTF-8 REGEX "^[^#]")
    foreach(setting ${zephyr_settings_txt})
      # Match <key>:<value> for each line of file, each corresponding to
      # a setting.  The use of quotes is required due to CMake not supporting
      # lazy regexes (it supports greedy only).
      string(REGEX REPLACE "\"(.*)\":\".*\"" "\\1" key ${setting})
      string(REGEX REPLACE "\".*\":\"(.*)\"" "\\1" value ${setting})
      list(APPEND ${key} ${value})
    endforeach()
  endif()

  # Append ZEPHYR_BASE as a default ext root at lowest priority
  list(APPEND MODULE_EXT_ROOT ${ZEPHYR_BASE})

  if(EXISTS ${cmake_modules_file})
    file(STRINGS ${cmake_modules_file} zephyr_modules_txt ENCODING UTF-8)
  endif()

  set(ZEPHYR_MODULE_NAMES)
  foreach(module ${zephyr_modules_txt})
    # Match "<name>":"<path>" for each line of file, each corresponding to
    # one module. The use of quotes is required due to CMake not supporting
    # lazy regexes (it supports greedy only).
    string(REGEX REPLACE "\"(.*)\":\".*\":\".*\"" "\\1" module_name ${module})
    list(APPEND ZEPHYR_MODULE_NAMES ${module_name})
  endforeach()

  if(EXISTS ${cmake_sysbuild_file})
    file(STRINGS ${cmake_sysbuild_file} sysbuild_modules_txt ENCODING UTF-8)
  endif()

  set(SYSBUILD_MODULE_NAMES)
  foreach(module ${sysbuild_modules_txt})
    # Match "<name>":"<path>" for each line of file, each corresponding to
    # one module. The use of quotes is required due to CMake not supporting
    # lazy regexes (it supports greedy only).
    string(REGEX REPLACE "\"(.*)\":\".*\":\".*\"" "\\1" module_name ${module})
    list(APPEND SYSBUILD_MODULE_NAMES ${module_name})
  endforeach()

  # MODULE_EXT_ROOT is process order which means Zephyr module roots processed
  # later wins. therefore we reverse the list before processing.
  list(REVERSE MODULE_EXT_ROOT)
  foreach(root ${MODULE_EXT_ROOT})
    set(module_cmake_file_path modules/modules.cmake)
    if(NOT EXISTS ${root}/${module_cmake_file_path})
      message(FATAL_ERROR "No `${module_cmake_file_path}` found in module root `${root}`.")
    endif()

    include(${root}/${module_cmake_file_path})
  endforeach()

  foreach(module ${zephyr_modules_txt})
    # Match "<name>":"<path>" for each line of file, each corresponding to
    # one Zephyr module. The use of quotes is required due to CMake not
    # supporting lazy regexes (it supports greedy only).
    string(CONFIGURE ${module} module)
    string(REGEX REPLACE "\"(.*)\":\".*\":\".*\"" "\\1" module_name ${module})
    string(REGEX REPLACE "\".*\":\"(.*)\":\".*\"" "\\1" module_path ${module})
    string(REGEX REPLACE "\".*\":\".*\":\"(.*)\"" "\\1" cmake_path ${module})

    zephyr_string(SANITIZE TOUPPER MODULE_NAME_UPPER ${module_name})
    if(NOT ${MODULE_NAME_UPPER} STREQUAL CURRENT)
      set(ZEPHYR_${MODULE_NAME_UPPER}_MODULE_DIR ${module_path})
      set(ZEPHYR_${MODULE_NAME_UPPER}_CMAKE_DIR ${cmake_path})
    else()
      message(FATAL_ERROR "Found Zephyr module named: ${module_name}\n\
${MODULE_NAME_UPPER} is a restricted name for Zephyr modules as it is used for \
\${ZEPHYR_${MODULE_NAME_UPPER}_MODULE_DIR} CMake variable.")
    endif()
  endforeach()

  foreach(module ${sysbuild_modules_txt})
    # Match "<name>":"<path>" for each line of file, each corresponding to
    # one Zephyr module. The use of quotes is required due to CMake not
    # supporting lazy regexes (it supports greedy only).
    string(CONFIGURE ${module} module)
    string(REGEX REPLACE "\"(.*)\":\".*\":\".*\"" "\\1" module_name ${module})
    string(REGEX REPLACE "\".*\":\"(.*)\":\".*\"" "\\1" module_path ${module})
    string(REGEX REPLACE "\".*\":\".*\":\"(.*)\"" "\\1" cmake_path ${module})

    zephyr_string(SANITIZE TOUPPER MODULE_NAME_UPPER ${module_name})
    if(NOT ${MODULE_NAME_UPPER} STREQUAL CURRENT)
      set(SYSBUILD_${MODULE_NAME_UPPER}_MODULE_DIR ${module_path})
      set(SYSBUILD_${MODULE_NAME_UPPER}_CMAKE_DIR ${cmake_path})
    else()
      message(FATAL_ERROR "Found Zephyr module named: ${module_name}\n\
${MODULE_NAME_UPPER} is a restricted name for Zephyr modules as it is used for \
\${SYSBUILD_${MODULE_NAME_UPPER}_MODULE_DIR} CMake variable.")
    endif()
  endforeach()
else()

  file(WRITE ${kconfig_modules_file}
    "# No west and no Zephyr modules\n"
    )

  file(WRITE ${kconfig_sysbuild_file}
    "# No west and no Zephyr modules\n"
    )

endif()
