# SPDX-License-Identifier: Apache-2.0


add_library(PTHREAD INTERFACE)

if(CONFIG_POSIX_HEADERS)
zephyr_include_directories(${ZEPHYR_BASE}/include/posix)
endif()

zephyr_library()
zephyr_library_sources(fdtable.c)
zephyr_library_sources(pthread_common.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_cond.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_mutex.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_barrier.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread.c)
zephyr_library_sources(pthread_sched.c)
zephyr_library_sources(clock.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_SLEEP sleep.c)
zephyr_library_sources(timer.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_rwlock.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC semaphore.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_key.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_MQUEUE mqueue.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_FS fs.c)

zephyr_library_link_libraries(PTHREAD)
target_link_libraries(PTHREAD INTERFACE zephyr_interface)
