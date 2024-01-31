# SPDX-License-Identifier: Apache-2.0

set(GEN_DIR ${ZEPHYR_BINARY_DIR}/include/generated)

zephyr_syscall_header(
  posix_clock.h
)

if(CONFIG_POSIX_API)
  zephyr_include_directories(${ZEPHYR_BASE}/include/zephyr/posix)
endif()

if(CONFIG_POSIX_SIGNAL)
  set(STRSIGNAL_TABLE_H ${GEN_DIR}/posix/strsignal_table.h)

  add_custom_command(
    OUTPUT ${STRSIGNAL_TABLE_H}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/gen_strsignal_table.py
    -i ${ZEPHYR_BASE}/include/zephyr/posix/signal.h
    -o ${STRSIGNAL_TABLE_H}
    DEPENDS ${ZEPHYR_BASE}/include/zephyr/posix/signal.h
  )
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
zephyr_library_sources_ifdef(CONFIG_POSIX_FS fs.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_MQUEUE mqueue.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_PUTMSG stropts.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_SIGNAL signal.c ${STRSIGNAL_TABLE_H})
zephyr_library_sources_ifdef(CONFIG_POSIX_UNAME uname.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC _common.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_BARRIER barrier.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_COND cond.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_KEY key.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_MUTEX mutex.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD pthread.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_RWLOCK rwlock.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_PRIORITY_SCHEDULING sched.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC semaphore.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_SPINLOCK spinlock.c)
zephyr_library_sources_ifdef(CONFIG_TIMER timer.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
)
