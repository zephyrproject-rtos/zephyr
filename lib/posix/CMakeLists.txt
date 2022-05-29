# SPDX-License-Identifier: Apache-2.0


zephyr_interface_library_named(posix_subsys)

if(CONFIG_POSIX_API OR CONFIG_PTHREAD_IPC OR CONFIG_POSIX_CLOCK
    OR CONFIG_POSIX_MQUEUE OR CONFIG_POSIX_FS OR CONFIG_EVENTFD
    OR CONFIG_GETOPT OR CONFIG_NET_SOCKETS_POSIX_NAMES)
  zephyr_include_directories(${ZEPHYR_BASE}/include/zephyr/posix)
  if(CONFIG_NEWLIB_LIBC)
    # Avoid "error: conflicting types for.. sys/_pthreadtypes.h .. previous declaration was here"
    zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:-D_SYS__PTHREADTYPES_H_>)
    zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:-D_SYS__PTHREADTYPES_H_>)
    # Avoid "error: conflicting types for.. sys/_pthreadtypes.h .. previous declaration was here"
    zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:-DMISSING_SYSCALL_NAMES>)
    zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:-DMISSING_SYSCALL_NAMES>)
    if(CONFIG_POSIX_CLOCK)
      # Allow newlib's <time.h> to define clock_settime() et al
      zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:-D_POSIX_TIMERS -D_POSIX_MONOTONIC_CLOCK>)
      zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:-D_POSIX_TIMERS -D_POSIX_MONOTONIC_CLOCK>)
    endif()
  endif()
endif()

zephyr_library()
zephyr_library_sources(perror.c)
zephyr_library_sources(pthread_common.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_cond.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_mutex.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_barrier.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_sched.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_CLOCK clock.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_CLOCK sleep.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_CLOCK timer.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_rwlock.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC semaphore.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_key.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_MQUEUE mqueue.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_FS fs.c)
zephyr_library_sources_ifdef(CONFIG_EVENTFD eventfd.c)
add_subdirectory_ifdef(CONFIG_GETOPT getopt)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
)

zephyr_library_link_libraries(posix_subsys)
