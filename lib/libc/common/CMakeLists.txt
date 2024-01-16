# SPDX-License-Identifier: Apache-2.0

zephyr_system_include_directories(include)

add_library(c_common)
target_link_libraries(c_common PUBLIC zephyr_interface kernel)

if(CONFIG_MINIMAL_LIBC OR CONFIG_PICOLIBC)
  target_link_libraries(kernel INTERFACE c_common)
else()
  zephyr_append_cmake_library(c_common)
endif()

target_sources_ifdef(CONFIG_COMMON_LIBC_ABORT c_common PRIVATE source/stdlib/abort.c)
target_sources_ifdef(CONFIG_COMMON_LIBC_ASCTIME c_common PRIVATE source/time/asctime.c)
target_sources_ifdef(CONFIG_COMMON_LIBC_GMTIME_R c_common PRIVATE source/time/gmtime_r.c)
target_sources_ifdef(CONFIG_COMMON_LIBC_LOCALTIME_R_UTC c_common PRIVATE source/time/localtime_r_utc.c)
target_sources_ifdef(CONFIG_COMMON_LIBC_CTIME c_common PRIVATE source/time/ctime.c)
target_sources_ifdef(CONFIG_COMMON_LIBC_TIME c_common PRIVATE source/time/time.c)
target_sources_ifdef(CONFIG_COMMON_LIBC_STRNLEN c_common PRIVATE source/string/strnlen.c)
target_sources_ifdef(CONFIG_COMMON_LIBC_THRD c_common PRIVATE
  source/thrd/cnd.c
  source/thrd/mtx.c
  source/thrd/once.c
  source/thrd/thrd.c
  source/thrd/tss.c
  )

if(CONFIG_COMMON_LIBC_MALLOC)
  target_sources(c_common PRIVATE source/stdlib/malloc.c)

  # Prevent compiler from optimizing calloc into an infinite recursive call
  target_compile_options(c_common PRIVATE $<TARGET_PROPERTY:compiler,no_builtin_malloc>)

  # Make sure that any allocation calls from within the C library get
  # routed back to the Zephyr versions by mapping them to z_ aliases
  target_link_options(zephyr_interface INTERFACE
    ${LINKERFLAGPREFIX},--wrap=aligned_alloc
    ${LINKERFLAGPREFIX},--wrap=free
    ${LINKERFLAGPREFIX},--wrap=malloc
    ${LINKERFLAGPREFIX},--wrap=realloc
    )
zephyr_library_sources_ifdef(CONFIG_COMMON_LIBC_REMOVE source/stdio/remove.c)

  if(CONFIG_GLIBCXX_LIBCPP)
    target_link_options(zephyr_interface INTERFACE
      ${LINKERFLAGPREFIX},--wrap=memalign
      )
  endif()

  if(CONFIG_COMMON_LIBC_CALLOC)
    target_link_options(zephyr_interface INTERFACE
      ${LINKERFLAGPREFIX},--wrap=calloc
      )
  endif()

  if(CONFIG_COMMON_LIBC_REALLOCARRAY)
    target_link_options(zephyr_interface INTERFACE
      ${LINKERFLAGPREFIX},--wrap=reallocarray
      )
  endif()
endif()
