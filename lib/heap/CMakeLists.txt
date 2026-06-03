# SPDX-License-Identifier: Apache-2.0

zephyr_constants_library(
  NAME heap_constants
  INCLUDES ${ARCH_DIR}/common/include
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/heap_constants.c
)

zephyr_sources(
  heap.c
  )

zephyr_sources_ifdef(CONFIG_SYS_HEAP_RUNTIME_STATS heap_stats.c)
zephyr_sources_ifdef(CONFIG_SYS_HEAP_INFO heap_info.c)
zephyr_sources_ifdef(CONFIG_SYS_HEAP_VALIDATE heap_validate.c)
zephyr_sources_ifdef(CONFIG_SYS_HEAP_STRESS heap_stress.c)
zephyr_sources_ifdef(CONFIG_SHARED_MULTI_HEAP shared_multi_heap.c)
zephyr_sources_ifdef(CONFIG_MULTI_HEAP multi_heap.c)
zephyr_sources_ifdef(CONFIG_HEAP_LISTENER heap_listener.c)
zephyr_sources_ifdef(CONFIG_SYS_HEAP_ARRAY_SIZE heap_array.c)
zephyr_sources_ifdef(CONFIG_SYS_HEAP_SANITIZER_ASAN heap_sanitizer_asan.c)
zephyr_sources_ifdef(CONFIG_SYS_HEAP_KASAN heap_kasan.c)

if(CONFIG_SYS_HEAP_SANITIZER_ASAN)
  # The heap implementation owns the bytes it poisons (chunk headers,
  # free-list links, canaries) and must read and write them freely, so the
  # heap internal sources are excluded from ASAN instrumentation. The
  # public-API hooks still call the __asan_* runtime to poison and unpoison
  # the user-visible regions so instrumented application code is checked.
  # These sources are added to the root-scoped "zephyr" target, so the
  # COMPILE_OPTIONS property must be set in that target's directory scope.
  set_property(SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/heap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/heap_stats.c
    ${CMAKE_CURRENT_SOURCE_DIR}/heap_info.c
    ${CMAKE_CURRENT_SOURCE_DIR}/heap_validate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/heap_stress.c
    ${CMAKE_CURRENT_SOURCE_DIR}/heap_array.c
    TARGET_DIRECTORY zephyr
    APPEND PROPERTY COMPILE_OPTIONS -fno-sanitize=address)
endif()

if(CONFIG_SYS_HEAP_KASAN)
  get_property(_heap_kasan_flags TARGET compiler PROPERTY heap_kasan)
  if(NOT _heap_kasan_flags)
    message(FATAL_ERROR
      "CONFIG_SYS_HEAP_KASAN has only been supported with GCC and Clang")
  endif()
  get_property(_no_heap_kasan TARGET compiler PROPERTY no_heap_kasan)
  set_property(SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/heap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/heap_kasan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/heap_validate.c
    TARGET_DIRECTORY zephyr
    APPEND PROPERTY COMPILE_OPTIONS "${_no_heap_kasan}")
endif()
