# SPDX-License-Identifier: Apache-2.0


zephyr_interface_library_named(posix_subsys)

if(CONFIG_POSIX_API)
  target_include_directories(posix_subsys INTERFACE ${ZEPHYR_BASE}/include/zephyr/posix)
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

if(NOT (CONFIG_BOARD_NATIVE_POSIX OR CONFIG_BOARD_NATIVE_POSIX_64BIT))
zephyr_library_sources(nanosleep.c)
endif()

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
)

zephyr_library_link_libraries(posix_subsys)
