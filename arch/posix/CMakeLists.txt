# SPDX-License-Identifier: Apache-2.0

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${CMAKE_HOST_SYSTEM_NAME}.${CMAKE_HOST_SYSTEM_PROCESSOR}.cmake)
  # @Intent: Set necessary compiler & linker options for this specific host architecture & OS
  include(${CMAKE_HOST_SYSTEM_NAME}.${CMAKE_HOST_SYSTEM_PROCESSOR}.cmake)
else()
  if (CONFIG_64BIT)
    # some gcc versions fail to build without -fPIC
    zephyr_compile_options(-m64 -fPIC)
    zephyr_link_libraries(-m64)
  else ()
    zephyr_compile_options(-m32)
    zephyr_link_libraries(-m32)
  endif ()
endif()

zephyr_compile_options(
  ${ARCH_FLAG}
  -include ${ZEPHYR_BASE}/arch/posix/include/posix_cheats.h
  )

# @Intent: Obtain compiler specific flags for no freestanding compilation
zephyr_compile_options($<TARGET_PROPERTY:compiler,hosted>)

zephyr_include_directories(${BOARD_DIR})

if (CONFIG_COVERAGE)
  zephyr_compile_options($<TARGET_PROPERTY:compiler,coverage>)
  zephyr_link_libraries($<TARGET_PROPERTY:linker,coverage>)
endif ()

if (CONFIG_ASAN)
  zephyr_compile_options($<TARGET_PROPERTY:compiler,sanitize_address>)
  zephyr_link_libraries($<TARGET_PROPERTY:linker,sanitize_address>)
endif ()

if (CONFIG_UBSAN)
  zephyr_compile_options($<TARGET_PROPERTY:compiler,sanitize_undefined>)
  zephyr_link_libraries($<TARGET_PROPERTY:linker,sanitize_undefined>)
endif ()

zephyr_compile_definitions(_POSIX_C_SOURCE=200809 _XOPEN_SOURCE=600 _XOPEN_SOURCE_EXTENDED)

zephyr_ld_options(
  -ldl
  -pthread
)

# About the -include directive: The reason to do it this way, is because in this
# manner it is transparent to the application. Otherwise posix_cheats.h needs to
# be included in all the applications' files which define main( ), and in any
# app file which uses the pthreads like API provided by Zephyr
# ( include/posix/pthread.h / kernel/pthread.c ) [And any future API added to
# Zephyr which will clash with the native POSIX API] . It would also need to
# be included in a few zephyr kernel files.


add_subdirectory(core)
