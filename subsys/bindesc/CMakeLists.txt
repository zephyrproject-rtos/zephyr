# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_X86)
  zephyr_linker_sources(ROM_START SORT_KEY 0x1bindesc bindesc_no_vt.ld)
  zephyr_library_sources(x86/bindesc_skip.S)
else()
  zephyr_linker_sources(ROM_START SORT_KEY 0x1bindesc bindesc.ld)
endif()

# Wrapper macro around string(TIMESTAMP ...), that returns the time
# in either local time or UTC, depending on CONFIG_BINDESC_BUILD_TIME_USE_LOCAL_TIME.
macro(get_time out_var format)
  if(CONFIG_BINDESC_BUILD_TIME_USE_LOCAL_TIME)
    string(TIMESTAMP ${out_var} ${format})
  else()
    string(TIMESTAMP ${out_var} ${format} UTC)
  endif()
endmacro()

macro(gen_build_time_int_definition def_name format)
  if(CONFIG_BINDESC_${def_name})
    get_time(${def_name} ${format})
    # remove leading zeros so that the output will not be interpreted as octal
    math(EXPR ${def_name} ${${def_name}})
    zephyr_library_compile_definitions(${def_name}=${${def_name}})
  endif()
endmacro()

macro(gen_build_time_str_definition def_name format)
  if(CONFIG_BINDESC_${def_name})
    get_time(${def_name} ${${format}})
    zephyr_library_compile_definitions(${def_name}="${${def_name}}")
  endif()
endmacro()

macro(gen_str_definition def_name value)
  if(CONFIG_BINDESC_${def_name})
    zephyr_library_compile_definitions(${def_name}="${value}")
  endif()
endmacro()

if(CONFIG_BINDESC_DEFINE_BUILD_TIME)
  zephyr_library_sources(bindesc_build_time.c)
  gen_build_time_int_definition(BUILD_TIME_YEAR "%Y")
  gen_build_time_int_definition(BUILD_TIME_MONTH "%m")
  gen_build_time_int_definition(BUILD_TIME_DAY "%d")
  gen_build_time_int_definition(BUILD_TIME_HOUR "%H")
  gen_build_time_int_definition(BUILD_TIME_MINUTE "%M")
  gen_build_time_int_definition(BUILD_TIME_SECOND "%S")
  gen_build_time_int_definition(BUILD_TIME_UNIX "%s")

  gen_build_time_str_definition(BUILD_DATE_TIME_STRING
      CONFIG_BINDESC_BUILD_DATE_TIME_STRING_FORMAT)
  gen_build_time_str_definition(BUILD_DATE_STRING
      CONFIG_BINDESC_BUILD_DATE_STRING_FORMAT)
  gen_build_time_str_definition(BUILD_TIME_STRING
      CONFIG_BINDESC_BUILD_TIME_STRING_FORMAT)

  if(CONFIG_BINDESC_BUILD_TIME_ALWAYS_REBUILD)
    # By adding a custom target that invokes cmake,
    # CMake is forced to rebuild this target on every build. This is
    # done to ensure that the timestamp is always up to date.
    add_custom_target(
      bindesc_time_force_rebuild
      COMMAND ${CMAKE_COMMAND} ${CMAKE_BINARY_DIR}
    )
    zephyr_library_add_dependencies(bindesc_time_force_rebuild)
  endif()
endif()

if(CONFIG_BINDESC_DEFINE_VERSION)
  zephyr_library_sources(bindesc_version.c)
  if(EXISTS ${APPLICATION_SOURCE_DIR}/VERSION)
    zephyr_library_compile_definitions(HAS_APP_VERSION=1)
  endif()
endif()

if(CONFIG_BINDESC_DEFINE_HOST_INFO)
  cmake_host_system_information(RESULT hostname QUERY HOSTNAME)
  zephyr_library_sources(bindesc_host_info.c)
  gen_str_definition(HOST_NAME ${hostname})
  gen_str_definition(C_COMPILER_NAME ${CMAKE_C_COMPILER_ID})
  gen_str_definition(C_COMPILER_VERSION ${CMAKE_C_COMPILER_VERSION})
  gen_str_definition(CXX_COMPILER_NAME ${CMAKE_CXX_COMPILER_ID})
  gen_str_definition(CXX_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
endif()
