# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(
  ${ZEPHYR_BASE}/include/zephyr/posix/time.h
)

zephyr_interface_library_named(posix_subsys)

if(CONFIG_POSIX_API)
  zephyr_include_directories(${ZEPHYR_BASE}/include/zephyr/posix)
endif()

if(CONFIG_POSIX_API OR CONFIG_PTHREAD_IPC OR CONFIG_POSIX_CLOCK OR
  CONFIG_POSIX_MQUEUE OR CONFIG_POSIX_FS OR CONFIG_EVENTFD OR CONFIG_GETOPT)
  # This is a temporary workaround so that Newlib declares the appropriate
  # types for us. POSIX features to be formalized as part of #51211
  zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:-D_POSIX_THREADS>)
  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:-D_POSIX_THREADS>)
endif()

zephyr_library()
add_subdirectory_ifdef(CONFIG_GETOPT getopt)
zephyr_library_sources_ifdef(CONFIG_EVENTFD eventfd.c)
zephyr_library_sources_ifdef(CONFIG_FNMATCH fnmatch.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_API perror.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_CLOCK clock.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_CLOCK nanosleep.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_CLOCK sleep.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_CLOCK timer.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_FS fs.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_MQUEUE mqueue.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_UNAME uname.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC _common.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC barrier.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC cond.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC key.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC mutex.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC rwlock.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC sched.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC semaphore.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC spinlock.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
)

zephyr_library_link_libraries(posix_subsys)
zephyr_library_property(ALLOW_EMPTY TRUE)
