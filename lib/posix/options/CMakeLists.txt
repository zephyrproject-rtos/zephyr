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

if(CONFIG_POSIX_API OR CONFIG_PTHREAD_IPC OR CONFIG_POSIX_TIMERS OR
  CONFIG_POSIX_MESSAGE_PASSING OR CONFIG_POSIX_FILE_SYSTEM OR CONFIG_EVENTFD OR
  CONFIG_POSIX_C_LIB_EXT OR CONFIG_POSIX_SINGLE_PROCESS)
  # This is a temporary workaround so that Newlib declares the appropriate
  # types for us. POSIX features to be formalized as part of #51211
  zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:-D_POSIX_THREADS>)
  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:-D_POSIX_THREADS>)
endif()

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_EVENTFD eventfd.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_ASYNCHRONOUS_IO aio.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_BARRIERS barrier.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_C_LIB_EXT
  fnmatch.c
  getentropy.c
  getopt/getopt.c
  getopt/getopt_common.c
)
zephyr_library_sources_ifdef(CONFIG_POSIX_DEVICE_IO
  # perror should be moved to the common libc
  perror.c
  device_io.c
)
zephyr_library_sources_ifdef(CONFIG_POSIX_FD_MGMT
  fd_mgmt.c
)
zephyr_library_sources_ifdef(CONFIG_POSIX_FILE_SYSTEM fs.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_FSYNC fsync.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_MESSAGE_PASSING mqueue.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_MULTI_PROCESS
  sleep.c
)
zephyr_library_sources_ifdef(CONFIG_POSIX_NETWORKING net.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_PUTMSG stropts.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_SIGNAL signal.c ${STRSIGNAL_TABLE_H})
zephyr_library_sources_ifdef(CONFIG_POSIX_SINGLE_PROCESS
  confstr.c
  env.c
  sysconf.c
  uname.c
)
zephyr_library_sources_ifdef(CONFIG_POSIX_SPIN_LOCKS spinlock.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_SYSLOG syslog.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_TIMERS
  clock.c
  timer.c
)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC _common.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_COND cond.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_KEY key.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_MUTEX mutex.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD pthread.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_PRIORITY_SCHEDULING sched.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_READER_WRITER_LOCKS rwlock.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_SEMAPHORES semaphore.c)

zephyr_library_sources_ifdef(CONFIG_GETOPT_LONG
  getopt/getopt_long.c
)
zephyr_include_directories_ifdef(CONFIG_POSIX_C_LIB_EXT
  getopt/
)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
)

zephyr_library_property(ALLOW_EMPTY TRUE)
