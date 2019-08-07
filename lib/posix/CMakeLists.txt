# SPDX-License-Identifier: Apache-2.0


add_library(posix_subsys INTERFACE)

target_include_directories(posix_subsys INTERFACE ${ZEPHYR_BASE}/include/posix)

zephyr_library()
zephyr_library_sources(pthread_common.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_cond.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_mutex.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_barrier.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread.c)
zephyr_library_sources(pthread_sched.c)
zephyr_library_sources(clock.c)
zephyr_library_sources(sleep.c)
zephyr_library_sources(timer.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_rwlock.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC semaphore.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC pthread_key.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_MQUEUE mqueue.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_FS fs.c)

zephyr_library_link_libraries(posix_subsys)
target_link_libraries(posix_subsys INTERFACE zephyr_interface)
