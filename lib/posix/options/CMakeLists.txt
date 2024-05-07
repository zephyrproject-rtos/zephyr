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

zephyr_library()
add_subdirectory_ifdef(CONFIG_GETOPT getopt)
zephyr_library_sources_ifdef(CONFIG_EVENTFD eventfd.c)
zephyr_library_sources_ifdef(CONFIG_FNMATCH fnmatch.c)
zephyr_library_sources_ifdef(CONFIG_GETENTROPY getentropy.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_API perror.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_ASYNCHRONOUS_IO aio.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_FILE_SYSTEM fs.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_MESSAGE_PASSING mqueue.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_NETWORKING net.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_PUTMSG stropts.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_SIGNAL signal.c ${STRSIGNAL_TABLE_H})
zephyr_library_sources_ifdef(CONFIG_POSIX_SYSCONF_IMPL_FULL sysconf.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_SYSLOG syslog.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_THREADS_BASE
  cond.c
  key.c
  mutex.c
  pthread.c
)
zephyr_library_sources_ifdef(CONFIG_POSIX_BARRIERS barrier.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_READER_WRITER_LOCKS rwlock.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_PRIORITY_SCHEDULING sched.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_SEMAPHORES semaphore.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_SINGLE_PROCESS
  confstr.c
  env.c
  sysconf.c
  uname.c
)
zephyr_library_sources_ifdef(CONFIG_POSIX_SPIN_LOCKS spinlock.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_TIMERS
  clock.c
  timer.c
)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
)

zephyr_library_property(ALLOW_EMPTY TRUE)
