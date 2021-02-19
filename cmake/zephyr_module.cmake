# SPDX-License-Identifier: Apache-2.0

# This cmake file provides functionality to import CMakeLists.txt and Kconfig
# files for Zephyr modules into Zephyr build system.
#
# CMakeLists.txt and Kconfig files can reside directly in the module or in a
# MODULE_EXT_ROOT.
# The `<module>/zephyr/module.yml` file specifies whether the build files are
# located in the module or in a MODULE_EXT_ROOT.
#
# A list of Zephyr modules can be provided to the build system using:
#   -DZEPHYR_MODULES=<module-path>[;<additional-module(s)-path>]
#
# It looks for: <module>/zephyr/module.yml or
#               <module>/zephyr/CMakeLists.txt
# to load the module into Zephyr build system.
# If west is available, it uses `west list` to obtain a list of projects to
# search for zephyr/module.yml
#
# If the module.yml file specifies that build files are located in a
# MODULE_EXT_ROOT then the variables:
# - `ZEPHYR_<MODULE_NAME>_CMAKE_DIR` is used for inclusion of the CMakeLists.txt
# - `ZEPHYR_<MODULE_NAME>_KCONFIG` is used for inclusion of the Kconfig
# files into the build system.

if(ZEPHYR_MODULES)
  set(ZEPHYR_MODULES_ARG "--modules" ${ZEPHYR_MODULES})
endif()

if(ZEPHYR_EXTRA_MODULES)
  set(ZEPHYR_EXTRA_MODULES_ARG "--extra-modules" ${ZEPHYR_EXTRA_MODULES})
endif()

set(KCONFIG_MODULES_FILE ${KCONFIG_BINARY_DIR}/Kconfig.modules)
set(ZEPHYR_SETTINGS_FILE ${CMAKE_BINARY_DIR}/zephyr_settings.txt)

if(WEST)
  set(WEST_ARG "--zephyr-base" ${ZEPHYR_BASE})
endif()

if(WEST OR ZEPHYR_MODULES)
  # Zephyr module uses west, so only call it if west is installed or
  # ZEPHYR_MODULES was provided as argument to CMake.
  execute_process(
    COMMAND
    ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/zephyr_module.py
    ${WEST_ARG}
    ${ZEPHYR_MODULES_ARG}
    ${ZEPHYR_EXTRA_MODULES_ARG}
    --kconfig-out ${KCONFIG_MODULES_FILE}
    --cmake-out ${CMAKE_BINARY_DIR}/zephyr_modules.txt
    --settings-out ${ZEPHYR_SETTINGS_FILE}
    WORKING_DIRECTORY ${ZEPHYR_BASE}
    ERROR_VARIABLE
    zephyr_module_error_text
    RESULT_VARIABLE
    zephyr_module_return
  )

  if(${zephyr_module_return})
      message(FATAL_ERROR "${zephyr_module_error_text}")
  endif()

  if(EXISTS ${ZEPHYR_SETTINGS_FILE})
    file(STRINGS ${ZEPHYR_SETTINGS_FILE} ZEPHYR_SETTINGS_TXT ENCODING UTF-8)
    foreach(setting ${ZEPHYR_SETTINGS_TXT})
      # Match <key>:<value> for each line of file, each corresponding to
      # a setting.  The use of quotes is required due to CMake not supporting
      # lazy regexes (it supports greedy only).
      string(REGEX REPLACE "\"(.*)\":\".*\"" "\\1" key ${setting})
      string(REGEX REPLACE "\".*\":\"(.*)\"" "\\1" value ${setting})
      # MODULE_EXT_ROOT is process order which means module roots processed
      # later wins. To ensure ZEPHYR_BASE stays first, and command line settings
      # are processed last, we insert at position 1.
      if ("${key}" STREQUAL "MODULE_EXT_ROOT")
        list(INSERT ${key} 1 ${value})
      else()
        list(APPEND ${key} ${value})
      endif()
    endforeach()
  endif()

  foreach(root ${MODULE_EXT_ROOT})
    if(NOT EXISTS ${root})
      message(FATAL_ERROR "No `modules.cmake` found in module root `${root}`.")
    endif()

    include(${root}/modules/modules.cmake)
  endforeach()

  if(EXISTS ${CMAKE_BINARY_DIR}/zephyr_modules.txt)
    file(STRINGS ${CMAKE_BINARY_DIR}/zephyr_modules.txt ZEPHYR_MODULES_TXT
         ENCODING UTF-8)
    set(ZEPHYR_MODULE_NAMES)

    foreach(module ${ZEPHYR_MODULES_TXT})
      # Match "<name>":"<path>" for each line of file, each corresponding to
      # one module. The use of quotes is required due to CMake not supporting
      # lazy regexes (it supports greedy only).
      string(CONFIGURE ${module} module)
      string(REGEX REPLACE "\"(.*)\":\".*\":\".*\"" "\\1" module_name ${module})
      string(REGEX REPLACE "\".*\":\"(.*)\":\".*\"" "\\1" module_path ${module})
      string(REGEX REPLACE "\".*\":\".*\":\"(.*)\"" "\\1" cmake_path ${module})

      list(APPEND ZEPHYR_MODULE_NAMES ${module_name})

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
  endif()
else()

  file(WRITE ${KCONFIG_MODULES_FILE}
    "# No west and no modules\n"
    )

endif()

if(DEFINED ZEPHYR_MODULE_NAMES)
  list(REMOVE_DUPLICATES ZEPHYR_MODULE_NAMES)
endif()
