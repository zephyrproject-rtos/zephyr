# SPDX-License-Identifier: Apache-2.0

file(GLOB cmake_modules "${CMAKE_CURRENT_LIST_DIR}/*/CMakeLists.txt")

foreach(module ${cmake_modules})
  get_filename_component(module_dir  ${module} DIRECTORY)
  get_filename_component(module_name ${module_dir} NAME)
  zephyr_string(SANITIZE TOUPPER MODULE_NAME_UPPER ${module_name})

  set(ZEPHYR_${MODULE_NAME_UPPER}_CMAKE_DIR ${module_dir})
endforeach()

file(GLOB kconfig_modules "${CMAKE_CURRENT_LIST_DIR}/*/Kconfig")

foreach(module ${kconfig_modules})
  get_filename_component(module_dir  ${module} DIRECTORY)
  get_filename_component(module_name ${module_dir} NAME)
  zephyr_string(SANITIZE TOUPPER MODULE_NAME_UPPER ${module_name})

  set(ZEPHYR_${MODULE_NAME_UPPER}_KCONFIG ${module_dir}/Kconfig)
endforeach()
