# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_X86)
  zephyr_linker_sources(ROM_START SORT_KEY 0x1bindesc bindesc_no_vt.ld)
  zephyr_library_sources(x86/bindesc_skip.S)
else()
  zephyr_linker_sources(ROM_START SORT_KEY 0x1bindesc bindesc.ld)
endif()

macro(gen_str_definition def_name value)
  if(CONFIG_BINDESC_${def_name})
    zephyr_library_compile_definitions(${def_name}="${value}")
  endif()
endmacro()

if(CONFIG_BINDESC_DEFINE_BUILD_TIME)
  zephyr_library_sources(bindesc_build_time.c)

  set(gen_header ${PROJECT_BINARY_DIR}/include/generated/bindesc_build_time.h)

  if(CONFIG_BINDESC_BUILD_TIME_USE_LOCAL_TIME)
    set(BUILD_TIME "LOCAL")
  else()
    set(BUILD_TIME "UTC")
  endif()

  set(GEN_COMMAND ${CMAKE_COMMAND}
      -DIN_FILE=${CMAKE_CURRENT_LIST_DIR}/bindesc_build_time.h.in
      -DOUT_FILE=${gen_header}
      -DBUILD_TIME_TYPE="${BUILD_TIME}"
      -DBUILD_DATE_TIME_STRING_FORMAT="${CONFIG_BINDESC_BUILD_DATE_TIME_STRING_FORMAT}"
      -DBUILD_DATE_STRING_FORMAT="${CONFIG_BINDESC_BUILD_DATE_STRING_FORMAT}"
      -DBUILD_TIME_STRING_FORMAT="${CONFIG_BINDESC_BUILD_TIME_STRING_FORMAT}"
      -P ${CMAKE_CURRENT_LIST_DIR}/gen_bindesc_build_time_h.cmake)

  if(CONFIG_BINDESC_BUILD_TIME_ALWAYS_REBUILD)
    add_custom_target(gen_bindesc_build_time
      COMMAND ${GEN_COMMAND}
      BYPRODUCTS ${gen_header}
    )
  else()
    add_custom_command(OUTPUT ${gen_header}
      COMMAND ${GEN_COMMAND}
    )
    add_custom_target(gen_bindesc_build_time DEPENDS ${gen_header})
  endif()

  zephyr_library_add_dependencies(gen_bindesc_build_time)
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
